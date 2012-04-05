/*===========================================================================
 *
 * task.h
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
 * Data structures related to task management.
 *
 *===========================================================================*/

#ifndef _SIMPLIX_TASK_H_
#define _SIMPLIX_TASK_H_

#include <simplix/consts.h>
#include <simplix/context.h>
#include <simplix/segment.h>
#include <simplix/types.h>

/* Task descriptor. */

struct task_struct {

    /* Task unique identifier. */
    pid_t pid;

    /* Parent task unique identifier. */
    pid_t ppid;

    /* CPU time used by this task, expressed in number of timer ticks. */
    unsigned long cputime;

    /* Time (in clock ticks) remaining before this task becomes runnable again.
       This is used by the sleep system call. */
    unsigned long timeout;

    /* Remaining time slice expressed in number of timer ticks.
       Always remains at 0 for the idle task. */
    unsigned int timeslice;

    /* The current state of this task. */
    int state;

    /* The exit code of this task, retrieved by its parent task. */
    int exit_status;

    /* Address of the task's kernel-space stack. */
    addr_t kstack;

    /* This task's local descriptor table (LDT) */
    struct segment_descriptor ldt[NR_LDT_ENTRIES];

    /* This task's CPU context i.e. the value of the stack pointer right
       before a task switch. */
    struct task_cpu_context *ctx;

    /* Linkage pointers in the global task list. */
    struct task_struct *prev, *next;
};

#endif /* _SIMPLIX_TASK_H_ */
