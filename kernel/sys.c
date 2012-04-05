/*===========================================================================
 *
 * sys.c
 *
 * Copyright (C) 2007 - Julien Lecomte
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *===========================================================================
 *
 * System calls.
 *
 *===========================================================================*/

#include <string.h>

#include <simplix/consts.h>
#include <simplix/context.h>
#include <simplix/globals.h>
#include <simplix/list.h>
#include <simplix/proto.h>
#include <simplix/segment.h>
#include <simplix/task.h>
#include <simplix/types.h>

long sys_exit(struct task_cpu_context *ctx)
{
    do_exit(ctx->ebx);

    /* The value returned here does not matter. The only reason why we return a
       value here is so that this function can have the same exact signature as
       all the other system calls. */
    return 0;
}

long sys_fork(struct task_cpu_context *ctx)
{
    struct task_struct *t = NULL;
    addr_t cur_cs_addr, cur_ds_addr, new_cs_addr, new_ds_addr;
    size_t cur_cs_size, cur_ds_size, new_cs_size, new_ds_size;

    /* Get a new task descriptor and initialize it. */
    t = kmalloc(sizeof(struct task_struct));
    if (!t) goto error;

    /* This guarantees that the init task has a known process id, even if
       kernel threads are spawned prior to the initial fork. */
    if (current->pid == IDLE_TASK_PID) {
        t->pid = INIT_TASK_PID;
    } else {
        t->pid = alloc_pid();
    }

    t->ppid = current->pid;
    t->state = TASK_RUNNABLE;
    t->timeslice = current->timeslice;

    /* Allocate some space for the task's kernel-space stack. We use the fast
       version of alloc_physmem_block because we overwrite the content of that
       memory area below. */
    if (__alloc_physmem_block(KSTACK_PAGES, &t->kstack) != S_OK)
        goto error;

    /* Copy the current task kernel-space stack. */
    memcpy((void *) t->kstack, (void *) current->kstack, KSTACK_SIZE);

    /* Get the address and size of the current task data segment. */
    cur_cs_addr = SEG_ADDR(&current->ldt[LDT_CS_INDEX]);
    cur_ds_addr = SEG_ADDR(&current->ldt[LDT_DS_INDEX]);
    if (cur_cs_addr != cur_ds_addr)
        panic("We cannot handle separate I&D space");

    cur_cs_size = SEG_SIZE(&current->ldt[LDT_CS_INDEX]);
    cur_ds_size = SEG_SIZE(&current->ldt[LDT_DS_INDEX]);
    if (cur_ds_size < cur_cs_size)
        panic("Invalid code or data segment size");

    /* Allocate some space for the new task's data segment. We use the fast
       version of alloc_physmem_block because we overwrite the content of that
       memory area below. */
    new_cs_size = cur_cs_size;
    new_ds_size = PAGE_ALIGN_SUP(cur_ds_size);
    if (__alloc_physmem_block(new_ds_size >> PAGE_BIT_SHIFT, &new_ds_addr) != S_OK)
        goto error;
    new_cs_addr = new_ds_addr;

    /* Copy the current task code and data segment. */
    memcpy((void *) new_ds_addr, (void *) cur_ds_addr, new_ds_size);

    /* Initialize the new task LDT. */
    t->ldt[LDT_CS_INDEX] = BUILD_4KB_SEG_DESC(new_cs_addr, new_cs_size, LDT_CS_TYPE);
    t->ldt[LDT_DS_INDEX] = BUILD_4KB_SEG_DESC(new_ds_addr, new_ds_size, LDT_DS_TYPE);

    /* Initialize the new task context. */
    t->ctx = (struct task_cpu_context *) (t->kstack + KSTACK_SIZE
        - sizeof(struct task_cpu_context));

    /* Set return value in EAX register for the new task. */
    t->ctx->eax = 0;

    /* Append the new task to the global task list. */
    list_append(task_list_head, t);

    printk("[pid %u] forking process -> new process has pid %u\n", current->pid, t->pid);
    return t->pid;

error:

    /* Free the memory we allocated. */
    if (new_ds_addr) free_physmem_block(new_ds_addr);
    if (t && t->kstack) free_physmem_block(t->kstack);
    if (t) kfree(t);
    return -1;
}

long sys_waitpid(struct task_cpu_context *ctx)
{
    pid_t pid;
    int status;
    addr_t vaddr, paddr;

    /* Compute the physical memory address of the variable that is going
       to receive the exit status of the task with the specified pid.
       Return -1 if the specified virtual address is invalid. */
    vaddr = ctx->ecx;
    if (!VALIDATE_VMEM_AREA(vaddr, sizeof(pid_t)))
        return -1;
    paddr = GET_PHYSMEM_ADDR(vaddr);

    /* Get the pid of the child we want to wait for. */
    pid = ctx->ebx;

    /* do_waitpid does most of the work. */
    pid = do_waitpid(pid, &status);
    if (pid != -1) {
        /* Copy the exit status to the appropriate physical address. */
        *((int *) paddr) = status;
    }

    /* Return the child's pid. */
    return pid;
}

long sys_getpid(struct task_cpu_context *ctx)
{
    return current->pid;
}

long sys_getppid(struct task_cpu_context *ctx)
{
    return current->ppid;
}

long sys_time(struct task_cpu_context *ctx)
{
    return realtime;
}

long sys_stime(struct task_cpu_context *ctx)
{
    addr_t vaddr, paddr;

    /* Compute the physical memory address of the variable containing the time.
       Return -1 if the specified virtual address is invalid. */
    vaddr = ctx->ecx;
    if (!VALIDATE_VMEM_AREA(vaddr, sizeof(time_t)))
        return -1;
    paddr = GET_PHYSMEM_ADDR(vaddr);

    /* Update the wall clock. */
    realtime = *((time_t *) paddr);

    return 0;
}

long sys_sleep(struct task_cpu_context *ctx)
{
    do_sleep(ctx->ebx);

    /* This system call cannot fail. */
    return 0;
}

long sys_brk(struct task_cpu_context *ctx)
{
    addr_t addr, cs_addr, ds_addr;
    size_t size, cs_size, ds_size;

    /* The new size for the data segment is stored in EBX. */
    size = PAGE_ALIGN_SUP(ctx->ebx);

    /* Get the address and size of the current task data segment. */
    cs_addr = SEG_ADDR(&current->ldt[LDT_CS_INDEX]);
    ds_addr = SEG_ADDR(&current->ldt[LDT_DS_INDEX]);
    if (cs_addr != ds_addr)
        panic("We cannot handle separate I&D space");

    cs_size = SEG_SIZE(&current->ldt[LDT_CS_INDEX]);
    ds_size = SEG_SIZE(&current->ldt[LDT_DS_INDEX]);
    if (ds_size < cs_size)
        panic("Invalid code or data segment size");

    if (size < cs_size) {
        /* Simplix tasks have common I&D spaces, so we cannot make the data
           segment smaller than the code segment. */
        return ds_size;
    }

    /* Do the actual reallocation. */
    if (realloc_physmem_block(ds_addr, size >> PAGE_BIT_SHIFT, &addr) != S_OK)
        return ds_size;

    /* Adjust the task's LDT. */
    current->ldt[LDT_CS_INDEX] = BUILD_4KB_SEG_DESC(addr, cs_size, LDT_CS_TYPE);
    current->ldt[LDT_DS_INDEX] = BUILD_4KB_SEG_DESC(addr, size, LDT_DS_TYPE);

    /* Return the current break value. */
    return size;
}
