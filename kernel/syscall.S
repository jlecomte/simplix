/*===========================================================================
 *
 * syscall.S
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
 *===========================================================================*/

#include <simplix/consts.h>
#include <simplix/context.h>
#include <simplix/segment.h>

/*
 * This is our system call handler. It is a regular exception handler, so it is
 * structured in the same way as the other exception handlers defined in isr.S.
 * However, some special processing is required here when calling the C handler.
 */

.globl syscall_handler

syscall_handler:

    /* Push a fake error code, so the stack looks exactly the same as for
       exceptions with a real error code pushed by the CPU onto the stack. */
    pushl $0

    /* At this point, the top of the stack contains:
     *
     *      ss                  esp + 20
     *      esp                 esp + 16
     *      flags               esp + 12
     *      cs                  esp +  8
     *      eip                 esp +  4
     *      fake error code     esp
     */

    save_context

    restore_system_segments

    /* Check the system call number. */
    cmp $NR_SYSCALLS, %eax
    jae bad_syscall_num

    /* Call the C handler, passing in the address of the task's user context,
       from which can be retrieved the arguments to the system call. */
    push %esp
    call *syscall_table(,%eax,4)
    add $4, %esp

    jmp ret_from_syscall


bad_syscall_num:

    mov $-E_NOSYS, %eax


ret_from_syscall:

    /* Save the return value. Note: 48(%esp) corresponds to the EAX register
       after the context is restored (see isr.S for a description of what the
       interrupted task kernel stack looks like) */
    mov %eax, 48(%esp)

    /* The following iret instruction will cause a switch to user space.
       Therefore, we need to update the esp0 member of the TSS, the LDT
       descriptor in the GDT, as well as the LDTR register, which is
       accomplished by a call to update_tss_ldt */
    push %esp
    call update_tss_ldt
    add $4, %esp

    restore_context

    iret


syscall_table:

    .long sys_exit      /* 0 */
    .long sys_fork      /* 1 */
    .long sys_waitpid   /* 2 */
    .long sys_getpid    /* 3 */
    .long sys_getppid   /* 4 */
    .long sys_time      /* 5 */
    .long sys_stime     /* 6 */
    .long sys_sleep     /* 7 */
    .long sys_brk       /* 8 */
