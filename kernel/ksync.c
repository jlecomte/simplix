/*===========================================================================
 *
 * ksync.c
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
 * Synchronization primitives used by the Simplix kernel. These are implemented
 * by temporarily disabling hardware interrupts. This is fine on a uniprocessor
 * system (which is the case of Simplix) On a multiprocessor system, we must
 * use some special processor instruction (typically XCHG on x86 processors)
 * that runs atomically.
 *
 *===========================================================================*/

#include <simplix/consts.h>
#include <simplix/globals.h>
#include <simplix/list.h>
#include <simplix/macros.h>
#include <simplix/proto.h>
#include <simplix/task.h>
#include <simplix/types.h>

/*
 * This structure represents a kernel semaphore. This is an opaque structure
 * i.e. as far as the rest of the kernel is concerned, its implementation
 * details are not important.
 */
struct ksema {

    /* The current value of this semaphore.
       It represents the number of units of the resource which are free. */
    unsigned int value;

    /* The ordered list of tasks waiting on this semaphore. */
    struct task_struct *waiting_task_list_head;
};

/*
 * A kernel mutex is implemented as a binary semaphore.
 */
typedef struct ksema kmutex;

/*
 * Creates a semaphore and initializes it with the specified integer value.
 */
struct ksema *ksema_init(unsigned int initval)
{
    struct ksema *sem = __kmalloc(sizeof(struct ksema));
    if (sem)
        sem->value = initval;
    return sem;
}

/*
 * Disposes of the specified semaphore.
 */
ret_t ksema_free(struct ksema *sem)
{
    unsigned long eflags;
    disable_hwint(eflags);
    if (!list_empty(sem->waiting_task_list_head)) {
        restore_hwint(eflags);
        return -E_BUSY;
    }
    kfree(sem);
    restore_hwint(eflags);
    return S_OK;
}

/*
 * Implements the DOWN operation on the specified kernel semaphore.
 * Do not call this function from an interrupt handler!
 */
void ksema_down(struct ksema *sem)
{
    unsigned long eflags;

    disable_hwint(eflags);

    if (!sem->value) {
        /* Append the current task to the semaphore's wait queue and sleep. */
        list_append(sem->waiting_task_list_head, current);
        current->state = TASK_UNINTERRUPTIBLE;
        schedule();
    }

    /* Decrement the value of the semaphore. */
    sem->value--;

    restore_hwint(eflags);
}

/*
 * Implements the UP operation on the specified kernel semaphore.
 */
void ksema_up(struct ksema *sem)
{
    unsigned long eflags;
    struct task_struct *t;

    disable_hwint(eflags);

    /* Increment the value of the semaphore. */
    sem->value++;

    /* Remove the first task from the semaphore's wait queue and wake it up! */
    if (!list_empty(sem->waiting_task_list_head)) {
        t = list_pop_head(sem->waiting_task_list_head);
        t->state = TASK_RUNNABLE;
    }

    restore_hwint(eflags);
}

/*
 * Creates a mutex and initializes it in the unlocked state.
 */
struct kmutex *kmutex_init(void)
{
    return (struct kmutex *) ksema_init(1);
}

/*
 * Disposes of the specified mutex.
 */
ret_t kmutex_free(struct kmutex *mut)
{
    return ksema_free((struct ksema *) mut);
}

/*
 * Locks the specified mutex.
 * Do not call this function from an interrupt handler!
 */
void kmutex_lock(struct kmutex *mut)
{
    ksema_down((struct ksema *) mut);
}

/*
 * Unlocks the specified mutex.
 */
void kmutex_unlock(struct kmutex *mut)
{
    ksema_up((struct ksema *) mut);
}
