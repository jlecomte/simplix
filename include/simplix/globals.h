/*===========================================================================
 *
 * globals.h
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
 * Global variables.
 *
 *===========================================================================*/

#ifndef _SIMPLIX_GLOBALS_H_
#define _SIMPLIX_GLOBALS_H_

#include <simplix/segment.h>
#include <simplix/task.h>
#include <simplix/tss.h>
#include <simplix/types.h>

/* Size of the physical memory. Storage for this is created in physmem.c */
extern size_t physmem_size;

/* Our low level system call handler (see in syscall.S) */
extern addr_t syscall_handler;

/* Global task list. Storage for this is created in sched.c */
extern struct task_struct *task_list_head;

/* The currently executing task. Storage for this is created in sched.c */
extern struct task_struct *current;

/* The idle task. Storage for this is created in sched.c */
extern struct task_struct *idle_task;

/* The Global Descriptor Table (GDT) used by Simplix (see gdt.c) */
extern struct segment_descriptor gdt[];

/* The single Task-State Segment (TSS) used by Simplix (see gdt.c) */
extern struct tss_struct tss;

/* Low level system call handler. Storage for this is created in syscall.S */
extern addr_t syscall_handler;

/* Time related globals (see timer.c) */
extern unsigned long ticks;
extern time_t realtime;

#endif /* _SIMPLIX_GLOBALS_H_ */
