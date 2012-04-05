/*===========================================================================
 *
 * task_switch.S
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

#include <simplix/context.h>

.globl task_switch

/*
 * This function save the context of the currently running task onto its
 * stack and restore the context of the specified task before resuming it.
 * Here is its corresponding C prototype:
 *
 * void task_switch(struct task_struct *t);
 */

task_switch:

    /* These are removed from the stack by iret. Note that the next
       instruction executed by this task after it is "awoken" is the
       ret instruction immediately following the iret instruction below. */

    pushf
    push %cs
    push $resume_eip

    /* Push a fake error code, so the stack looks exactly the same as for
       exceptions with a real error code pushed by the CPU onto the stack. */
    pushl $0

    /* Save the context of the currently running task onto its stack */
    save_context

    /* At this point, the stack looks like this:
     *
     *      to_task             esp + 72
     *      ret                 esp + 68
     *      flags               esp + 64
     *      cs                  esp + 60
     *      eip                 esp + 56
     *     (fake) error code    esp + 52
     *      eax                 esp + 48
     *      ecx                 esp + 44
     *      edx                 esp + 40
     *      ebx                 esp + 36
     *      esp                 esp + 32
     *      ebp                 esp + 28
     *      esi                 esp + 24
     *      edi                 esp + 20
     *      ss                  esp + 16
     *      gs                  esp + 12
     *      fs                  esp +  8
     *      es                  esp +  4
     *      ds                  esp
     */

    /* Save the current value of the stack pointer in current->ctx.
       We'll use that value later when we restart this task. */

    mov current, %eax
    push %eax
    call get_ctx_addr
    add $4, %esp
    mov %esp, (%eax)

    /* Update the pointer to the currently executing task. */

    mov 72(%esp), %eax
    mov %eax, current

    /* Switch to the newly elected task's kernel stack. */

    push %eax
    call get_ctx
    add $4, %esp
    mov %eax, %esp

    /* The following iret instruction might cause a switch to user space if the
       newly elected task was just created. Therefore, we need to update the
       esp0 member of the TSS, the LDT descriptor in the GDT, as well as the
       LDTR register, which is accomplished by a call to update_tss_ldt. Note
       however that if the newly elected task has already run before, the
       context passed to update_tss_ldt will not be an interrupt context,
       in which case update_tss_ldt won't do anything. */

    push %esp
    call update_tss_ldt
    add $4, %esp

    /* Restore the context of the specified task. The context
       is restored exactly in the same order as it was saved. */

    restore_context

    /* Now restart the specified task. Note that the iret instruction restores,
       from the values stored on the stack, the flags, cs and eip, and possibly
       ss and esp if a change of privilege is needed, effectively resuming the
       execution of the task where it left off. */

    iret

resume_eip:
    ret
