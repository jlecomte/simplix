/*===========================================================================
 *
 * sched.c
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
 * Multitasking subsytem.
 *
 *===========================================================================*/

#include <simplix/assert.h>
#include <simplix/consts.h>
#include <simplix/globals.h>
#include <simplix/list.h>
#include <simplix/proto.h>
#include <simplix/segment.h>
#include <simplix/task.h>
#include <simplix/tss.h>
#include <simplix/types.h>

/* Global task list. */
struct task_struct *task_list_head = NULL;

/* The current task. */
struct task_struct *current = NULL;

/* The idle task. */
struct task_struct *idle_task = NULL;

/*
 * Return the address of the stored context associated with the specified task.
 * This is called from assembly code (see task_switch.S)
 */
struct task_cpu_context *get_ctx(struct task_struct *t)
{
    return t->ctx;
}

/*
 * Return the address of the task_struct member holding the address of the
 * stored context associated with the specified task. This is called from
 * assembly code (see task_switch.S)
 */
struct task_cpu_context **get_ctx_addr(struct task_struct *t)
{
    return &t->ctx;
}

/*
 * Update the esp0 member of the TSS, the LDT descriptor in the GDT, as well as
 * the LDTR register, to point to the current task LDT. This function should be
 * called whenever we are getting ready to possibly switch to user mode (right
 * before a context restore + iret) Note: The context passed to this function
 * is the interrupt context, not the context stored in the task descriptor.
 */
void update_tss_ldt(struct task_cpu_context *ctx)
{
    uint16_t ldtr;

    if (SEG_REG_RPL(ctx->cs) == USER_PRIVILEGE_LEVEL) {
        /* We are indeed switching to user space. */
        tss.esp0 = current->kstack + KSTACK_SIZE;
        gdt[GDT_LDT_INDEX] = BUILD_SEG_DESC((addr_t) current->ldt,
            NR_LDT_ENTRIES * sizeof(struct segment_descriptor),
            GDT_LDT_TYPE);
        ldtr = SEG_REG_VAL(KERN_PRIVILEGE_LEVEL, FALSE, GDT_LDT_INDEX);
        asm("lldt %0" : : "r" (ldtr));
    }
}

/*
 * Allocates a new process id.
 */
pid_t alloc_pid(void)
{
    int i;
    bool_t b;
    static pid_t next_pid = INIT_TASK_PID;
    struct task_struct *t;
    unsigned long eflags;

    disable_hwint(eflags);

    do {
        b = FALSE;
        next_pid = next_pid < MAX_PID ? next_pid + 1 : INIT_TASK_PID;
        list_for_each(task_list_head, t, i)
            if (t->pid == next_pid) {
                b = TRUE;
                break;
            }
    } while (b);

    restore_hwint(eflags);
    return next_pid;
}

/*
 * Returns the task associated with the specified pid, or NULL if a task with
 * the specified pid cannot be found.
 */
struct task_struct *get_task(pid_t pid)
{
    int i;
    struct task_struct *t;
    unsigned long eflags;

    disable_hwint(eflags);

    list_for_each(task_list_head, t, i)
        if (t->pid == pid) {
            restore_hwint(eflags);
            return t;
        }

    restore_hwint(eflags);
    return NULL;
}

/*
 * Initializes the multitasking subsystem and creates the idle task from the
 * current context (a.k.a the bootstrap thread) When this function returns,
 * we are in user mode!
 */
void init_multitasking(void)
{
    uint16_t ldtr;
    struct task_struct *t;

    /* The end of the kernel's text section. */
    extern char __e_text;

    /* The idle task user-space stack. */
    extern char __idle_ustack;

    /* Our low level system call handler (see syscall.S) */
    extern addr_t syscall_handler;

    /* Get a new task descriptor for the idle task and initialize it. */
    t = kmalloc(sizeof(struct task_struct));
    if (!t) goto error;
    t->pid = IDLE_TASK_PID;
    t->ppid = -1;
    t->state = TASK_RUNNABLE;
    t->timeslice = 0;

    /* Allocate some space for the idle task's kernel-space stack. */
    if (alloc_physmem_block(KSTACK_PAGES, &t->kstack) != S_OK)
        goto error;

    /* Setup the idle task ldt. */
    t->ldt[LDT_CS_INDEX] = BUILD_4KB_SEG_DESC(0, (addr_t) &__e_text, LDT_CS_TYPE);
    t->ldt[LDT_DS_INDEX] = BUILD_4KB_SEG_DESC(0, (addr_t) &__idle_ustack, LDT_DS_TYPE);

    /* Set the esp0 member of the TSS and initialize the LDTR register so we
       can reenter kernel space once we have entered user space below
       (the following code is similar to update_tss_ldt) */
    tss.esp0 = t->kstack + KSTACK_SIZE;
    gdt[GDT_LDT_INDEX] = BUILD_SEG_DESC((addr_t) t->ldt,
        NR_LDT_ENTRIES * sizeof(struct segment_descriptor),
        GDT_LDT_TYPE);
    ldtr = SEG_REG_VAL(KERN_PRIVILEGE_LEVEL, FALSE, GDT_LDT_INDEX);
    asm("lldt %0" : : "r" (ldtr));

    /* Save the idle task reference. This avoids having to look it up later. */
    idle_task = t;

    /* Since the idle task is the first task we are creating, it will be the
       first task we will switch from, so its context will initialize itself
       at the first task switch. */

    /* Mark the idle task as the current task. */
    current = t;

    /* And apend it to the global task list. */
    list_append(task_list_head, t);

    /* Set the system call handler in the IDT. */
    idt_set_handler(SYSCALL_INT_NUM, (addr_t) &syscall_handler, USER_PRIVILEGE_LEVEL);

    /* Move to user space. This is some cool magic I learned by looking at the
       code of an early version of Linux, and by adapting it a bit. Note that
       this will enable interrupts: the EFLAGS register gets set to 0x200,
       which corresponds to IF = 1 */
    asm("mov %%esp, %%eax;"
        "pushl %0;"
        "pushl %%eax;"
        "pushl $0x200;"
        "pushl %1;"
        "pushl $1f;"
        "iret;"
        "1: mov %0, %%ax;"
        "mov %%ax, %%ds;"
        "mov %%ax, %%es;"
        "mov %%ax, %%fs;"
        "mov %%ax, %%gs;"
        :
        : "i" (LDT_DS),
          "i" (LDT_CS)
        : "eax");

    return;

error:

    panic("Initialization of multi-tasking subsystem failed.");
}

/*
 * Elects a new task to execute, among the list of runnable tasks, and switch
 * to that task. This scheduling algorithm is ridiculously simple and not
 * particularly efficient. It should however be fair to interactive jobs,
 * while doing its best to accomodate CPU intensive tasks.
 */
void schedule(void)
{
    int i, n = 0;
    struct task_struct *t, *next = NULL;
    unsigned long eflags;

    disable_hwint(eflags);

    /* Find the runnable task with the highest possible remaining time slice. */
    list_for_each(task_list_head, t, i)
        if (t->state == TASK_RUNNABLE && t->timeslice > n)
            n = t->timeslice, next = t;

    if (!n) {
        /* We could not find a runnable task with a remaining time slice.
           If the only runnable task is the idle task, give it the CPU.
           Otherwise, adjust the time slice of all tasks and give the CPU
           to one of the runnable tasks. */
        list_for_each(task_list_head, t, i)
            if (t->pid != IDLE_TASK_PID) {
                t->timeslice += TIMESLICE_INCREMENT;
                if (t->timeslice > MAX_TIMESLICE)
                    t->timeslice = MAX_TIMESLICE;
                if (t->state == TASK_RUNNABLE)
                    next = t;
            }
    }

    if (!next) {
        /* The idle task is the only runnable task. */
        next = idle_task;
    }

    if (next != current) {
        /* Finally, do the actual task switch. */
        task_switch(next);
    }

    restore_hwint(eflags);
}

void wake_up(struct task_struct *t)
{
    ASSERT(t->state == TASK_INTERRUPTIBLE || t->state == TASK_UNINTERRUPTIBLE);
    t->state = TASK_RUNNABLE;
}

void sleep_on()
{
    current->state = TASK_UNINTERRUPTIBLE;
    schedule();
}

void interruptible_sleep_on()
{
    current->state = TASK_INTERRUPTIBLE;
    schedule();
}
