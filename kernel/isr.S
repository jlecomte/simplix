/*===========================================================================
 *
 * isr.S
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

/* When an ISR starts executing, the stack looks like this:
 *
 *     <ss>                <esp + 20>
 *     <esp>               <esp + 16>
 *      flags               esp + 12
 *      cs                  esp +  8
 *      eip                 esp +  4
 *     (fake) error code    esp
 *
 * Note: the value of the EFLAGS, CS and EIP registers were pushed by the
 * CPU prior to servicing this interrupt. The value of the stack pointer
 * and stack segment are also automatically pushed by the CPU onto the
 * stack if, and only if, the servicing of this interrupt was accompanied
 * by a change in privilege level (the CPU compares the CS in the IDT entry
 * with the current CS). The iret instruction will restore these registers
 * (the CPU knows what to restore from the stack by comparing the current
 * CS with the value of CS stored in the stack)
 *
 * We then save the current context (see the save_context macro in context.h).
 * After that, the stack looks like this:
 *
 *     <ss>                <esp + 72>
 *     <esp>               <esp + 68>
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

.globl irq_wrapper_array
.globl exception_wrapper_array

/* ISR for an IRQ line on master PIC. */

.irp level,       \
    IRQ_TIMER,    \
    IRQ_KEYBOARD, \
    SLAVE_PIC,    \
    IRQ_COM2,     \
    IRQ_COM1,     \
    IRQ_LPT2,     \
    IRQ_FLOPPY,   \
    IRQ_LPT1

.irq_wrapper\level:

    /* Push a fake error code, so the stack looks exactly the same as for
       exceptions with a real error code pushed by the CPU onto the stack. */
    pushl $0

    /* At this point, the top of the stack contains:
     *
     *     <ss>                <esp + 20>
     *     <esp>               <esp + 16>
     *      flags               esp + 12
     *      cs                  esp +  8
     *      eip                 esp +  4
     *      fake error code     esp
     */

    save_context

    restore_system_segments

    /* Send EOI to master PIC so we can receive subsequent interrupts. Note
       that since the current context has been saved, the interrupt service
       routine can be safely interrupted. */
    movb $PIC_EOI, %al
    outb %al, $PIC1_CMD

    /* Call the interrupt handler, passing in the interrupted task's context.
       Note that the interrupt handling may be accompanied by a task switch.
       We don't handle task switching here (although we could) We only handle
       the passage between user mode and kernel mode. Task switching is done
       separately in task_switch.S. */
    push %esp
    leal irq_handler_array, %edi
    call *\level*4(%edi)
    add $4, %esp

    /* The following iret instruction might cause a switch to user space if the
       interrupted task was a user task. Therefore, we need to update the esp0
       member of the TSS, the LDT descriptor in the GDT, as well as the LDTR
       register, which is accomplished by a call to update_tss_ldt. */

    push %esp
    call update_tss_ldt
    add $4, %esp

    restore_context

    iret

.endr

/* ISR for an IRQ line on slave PIC. */

.irp level,           \
    IRQ_RT_CLOCK,     \
    MASTER_PIC,       \
    IRQ_AVAILABLE_1,  \
    IRQ_AVAILABLE_2,  \
    IRQ_PS2_MOUSE,    \
    IRQ_COPROCESSOR,  \
    IRQ_PRIMARY_IDE,  \
    IRQ_SECONDARY_IDE

.irq_wrapper\level:

    /* Push a fake error code, so the stack looks exactly the same as for
       exceptions with a real error code pushed by the CPU onto the stack. */
    pushl $0

    /* At this point, the top of the stack contains:
     *
     *     <ss>                <esp + 20>
     *     <esp>               <esp + 16>
     *      flags               esp + 12
     *      cs                  esp +  8
     *      eip                 esp +  4
     *      fake error code     esp
     */

    save_context

    restore_system_segments

    /* Send EOI to both PICs so we can receive subsequent interrupts. Note
       that since the current context has been saved, the interrupt service
       routine can be safely interrupted. */
    movb $PIC_EOI, %al
    outb %al, $PIC2_CMD
    outb %al, $PIC1_CMD

    /* Call the interrupt handler, passing in the interrupted task's context.
       Note that the interrupt handling may be accompanied by a task switch.
       We don't handle task switching here (although we could) We only handle
       the passage between user mode and kernel mode. Task switching is done
       separately in task_switch.S. */
    push %esp
    leal irq_handler_array, %edi
    call *\level*4(%edi)
    add $4, %esp

    /* The following iret instruction might cause a switch to user space if the
       interrupted task was a user task. Therefore, we need to update the esp0
       member of the TSS, the LDT descriptor in the GDT, as well as the LDTR
       register, which is accomplished by a call to update_tss_ldt. */

    push %esp
    call update_tss_ldt
    add $4, %esp

    restore_context

    iret

.endr

/* ISR for exceptions without error code. */

.irp level,                             \
    EXCEPT_DIVIDE_ERROR,                \
    EXCEPT_DEBUG,                       \
    EXCEPT_NMI_INTERRUPT,               \
    EXCEPT_BREAKPOINT,                  \
    EXCEPT_OVERFLOW,                    \
    EXCEPT_BOUND_RANGE_EXCEDEED,        \
    EXCEPT_INVALID_OPCODE,              \
    EXCEPT_DEVICE_NOT_AVAILABLE,        \
    EXCEPT_COPROCESSOR_SEGMENT_OVERRUN, \
    EXCEPT_INTEL_RESERVED_1,            \
    EXCEPT_FLOATING_POINT_ERROR,        \
    EXCEPT_MACHINE_CHECK,               \
    EXCEPT_INTEL_RESERVED_2,            \
    EXCEPT_INTEL_RESERVED_3,            \
    EXCEPT_INTEL_RESERVED_4,            \
    EXCEPT_INTEL_RESERVED_5,            \
    EXCEPT_INTEL_RESERVED_6,            \
    EXCEPT_INTEL_RESERVED_7,            \
    EXCEPT_INTEL_RESERVED_8,            \
    EXCEPT_INTEL_RESERVED_9,            \
    EXCEPT_INTEL_RESERVED_10,           \
    EXCEPT_INTEL_RESERVED_11,           \
    EXCEPT_INTEL_RESERVED_12,           \
    EXCEPT_INTEL_RESERVED_13,           \
    EXCEPT_INTEL_RESERVED_14

.exception_wrapper\level:

    /* Push a fake error code, so the stack looks exactly the same as for
       exceptions with a real error code pushed by the CPU onto the stack. */
    pushl $0

    /* At this point, the top of the stack contains:
     *
     *     <ss>                <esp + 20>
     *     <esp>               <esp + 16>
     *      flags               esp + 12
     *      cs                  esp +  8
     *      eip                 esp +  4
     *      fake error code     esp
     */

    save_context

    restore_system_segments

    /* Call the C handler, passing the task's context. */
    push %esp
    leal exception_handler_array, %edi
    call *\level*4(%edi)
    add $4, %esp

    /* The following iret instruction might cause a switch to user space if the
       interrupted task was a user task. Therefore, we need to update the esp0
       member of the TSS, the LDT descriptor in the GDT, as well as the LDTR
       register, which is accomplished by a call to update_tss_ldt, just in case
       we would be switching to a different task than the one that trigerred the
       exception. */

    push %esp
    call update_tss_ldt
    add $4, %esp

    restore_context

    iret

.endr

/* ISR for exceptions with error code. */

.irp level,                     \
    EXCEPT_INVALID_TSS,         \
    EXCEPT_SEGMENT_NOT_PRESENT, \
    EXCEPT_STACK_SEGMENT_FAULT, \
    EXCEPT_GENERAL_PROTECTION,  \
    EXCEPT_PAGE_FAULT,          \
    EXCEPT_ALIGNMENT_CHECK

.exception_wrapper\level:

    /* At this point, the top of the stack contains:
     *
     *     <ss>                <esp + 20>
     *     <esp>               <esp + 16>
     *      flags               esp + 12
     *      cs                  esp +  8
     *      eip                 esp +  4
     *      error code          esp
     */

    save_context

    restore_system_segments

    /* Call the C handler, passing the task's context. */
    push %esp
    leal exception_handler_array, %edi
    call *\level*4(%edi)
    add $4, %esp

    /* The following iret instruction might cause a switch to user space if the
       interrupted task was a user task. Therefore, we need to update the esp0
       member of the TSS, the LDT descriptor in the GDT, as well as the LDTR
       register, which is accomplished by a call to update_tss_ldt, just in case
       we would be switching to a different task than the one that trigerred the
       exception. */

    push %esp
    call update_tss_ldt
    add $4, %esp

    restore_context

    iret

.endr

/* Special ISR for the double fault exception. This avoids getting triple
   fault exceptions which lead to immediate reboot. Here, the machine will
   freeze, which would allow us to detect that something bad happened and
   maybe debug it. */

.irp level, EXCEPT_DOUBLE_FAULT
.exception_wrapper\level:
    1: hlt
    jmp 1b
.endr

/* Array of ISRs (used in irq.c and exception.c) */

irq_wrapper_array:
.irp level, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
.long .irq_wrapper\level
.endr

exception_wrapper_array:
.irp level,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, \
            16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 29, 30, 31
.long .exception_wrapper\level
.endr
