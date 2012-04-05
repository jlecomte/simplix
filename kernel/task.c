/*===========================================================================
 *
 * task.c
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
 * Routines related to task handling.
 *
 *===========================================================================*/

#include <string.h>

#include <simplix/assert.h>
#include <simplix/consts.h>
#include <simplix/context.h>
#include <simplix/globals.h>
#include <simplix/list.h>
#include <simplix/macros.h>
#include <simplix/proto.h>
#include <simplix/segment.h>
#include <simplix/task.h>
#include <simplix/types.h>

/*
 * Creates a new kernel-space task, also known as kernel thread.
 */
pid_t kernel_thread(task_entry_point_t fn)
{
    struct task_struct *t;
    unsigned long eflags;

    /* Get a new task descriptor and initialize it. */
    t = kmalloc(sizeof(struct task_struct));
    if (!t) goto error;
    t->pid = alloc_pid();
    t->ppid = current != NULL ? current->pid : -1;
    t->state = TASK_RUNNABLE;
    t->timeslice = INITIAL_TIMESLICE;

    /* Allocate some space for the task's stack. */
    if (alloc_physmem_block(KSTACK_PAGES, &t->kstack) != S_OK)
        goto error;

    /* Initialize the task context. */
    t->ctx = (struct task_cpu_context *) (t->kstack + KSTACK_SIZE - sizeof(struct task_cpu_context));
    t->ctx->eflags = 1 << 9;
    t->ctx->cs = GDT_CS;
    t->ctx->eip = (uint32_t) fn;
    t->ctx->ss = GDT_DS;
    t->ctx->gs = GDT_DS;
    t->ctx->fs = GDT_DS;
    t->ctx->es = GDT_DS;
    t->ctx->ds = GDT_DS;

    /* Append the new task to the global task list. */
    disable_hwint(eflags);
    list_append(task_list_head, t);
    restore_hwint(eflags);

    printk("New kernel thread created with pid %u\n", t->pid);

    /* And return the new task pid. */
    return t->pid;

error:

    /* Free the memory we allocated. */
    if (t && t->kstack) free_physmem_block(t->kstack);
    if (t) kfree(t);
    return -1;
}

/*
 * Terminates the current task with the specified error code.
 */
void do_exit(int status)
{
    int i;
    addr_t seg_addr;
    struct task_struct *t, *p;

    /* IF will be reset at the next task switch. */
    cli();

    if (current->pid == IDLE_TASK_PID)
        panic("The idle task is being terminated.");

    printk("[pid %u] exiting with status %u\n", current->pid, status);

    /* Change the task state and set its exit status. */
    current->state = TASK_DEAD;
    current->exit_status = status;

    /* If this task does not have a parent (case of kernel threads started
       at boot time), make the init task the parent task. */
    if (current->ppid == -1)
        current->ppid = INIT_TASK_PID;

    /* Re-parent children tasks. */
    list_for_each(task_list_head, t, i)
        if (t->ppid == current->pid)
            t->ppid = current->ppid;

    /* Free the resources associated with the current task. */
    free_physmem_block(current->kstack);

    if (current->ldt[LDT_CS_INDEX].type != 0) {
        /* This is a user task. */
        seg_addr = SEG_ADDR(&current->ldt[LDT_CS_INDEX]);
        free_physmem_block(seg_addr);
    }

    p = get_task(current->ppid);

    /* Note: kernel threads started at boot time don't have a parent! */
    if (p && p->state == TASK_INTERRUPTIBLE) {
        /* Wake up the parent task. */
        p->state = TASK_RUNNABLE;
    }

    /* Finally, give the CPU to another task. */
    schedule();
}

/*
 * Waits for the task with the specified pid.
 */
pid_t do_waitpid(pid_t pid, int *status)
{
    int i;
    bool_t found;
    struct task_struct *t;
    unsigned long eflags;

    printk("[pid %u] waiting for child with pid = %d\n", current->pid, pid);

repeat:

    found = FALSE;
    disable_hwint(eflags);

    list_for_each(task_list_head, t, i)
        if (t->ppid == current->pid && (pid == -1 || t->pid == pid)) {
            found = TRUE;
            if (t->state == TASK_DEAD) {
                list_remove(task_list_head, t);
                restore_hwint(eflags);
                *status = t->exit_status;
                pid = t->pid;
                kfree(t);
                printk("[pid %u] all resources used by pid = %d freed\n", current->pid, pid);
                return pid;
            }
        }

    restore_hwint(eflags);

    if (!found) {
        /* This task does not have a child with the specified pid,
           or does not have any children to wait for. */
        return -1;
    }

    interruptible_sleep_on();

    /* We were awoken. Check again! */
    goto repeat;
}

/*
 * Put the current task to sleep for at least msec milliseconds.
 */
void do_sleep(unsigned long msec)
{
    /* We use a 64-bit variable to deal with a possible overflow
       during the computation of the timeout below. */
    unsigned long long timeout;

    if (!msec)
        return;

    if (current->pid == IDLE_TASK_PID)
        panic("The idle task is trying to sleep.");

    timeout = (msec * HZ) / 1000;
    current->timeout = (unsigned long) timeout;
    sleep_on();
}
