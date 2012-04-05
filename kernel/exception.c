/*===========================================================================
 *
 * exception.c
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

#include <simplix/assert.h>
#include <simplix/consts.h>
#include <simplix/context.h>
#include <simplix/globals.h>
#include <simplix/macros.h>
#include <simplix/proto.h>
#include <simplix/types.h>

/* Array of ISRs (defined in isr.S) */
extern addr_t exception_wrapper_array[NR_EXCEPTIONS];

/* Array of exception handlers */
exception_handler_t exception_handler_array[NR_EXCEPTIONS] = { NULL, };

/*
 * Set/Unset standard exception handlers.
 */
void exception_set_handler(unsigned int numex, exception_handler_t fn)
{
    addr_t isr;

    ASSERT(numex < NR_EXCEPTIONS);

    exception_handler_array[numex] = fn;

    isr = fn == NULL ? (addr_t) NULL : exception_wrapper_array[numex];
    idt_set_handler(numex, isr, KERN_PRIVILEGE_LEVEL);
}

static void handle_exception(char *s, uint32_t esp)
{
    struct task_cpu_context *ctx = (struct task_cpu_context *) esp;

    printk("+------------------------------------------------------------\n");
    printk("| [pid %u] %s\n", current->pid, s);
    printk("| EAX=0x%.08x EBX=0x%.08x ECX=0x%.08x EDX=0x%.08x\n",
        ctx->eax, ctx->ebx, ctx->ecx, ctx->edx);
    printk("| ESP=0x%.08x EBP=0x%.08x ESI=0x%.08x EDI=0x%.08x\n",
        ctx->esp3, ctx->ebp, ctx->esi, ctx->edi);
    printk("| EIP=0x%.08x\n", ctx->eip);
    printk("|  CS=0x%.04x\n", ctx->cs);
    printk("|  DS=0x%.04x\n", ctx->ds);
    printk("|  ES=0x%.04x\n", ctx->es);
    printk("|  FS=0x%.04x\n", ctx->fs);
    printk("|  GS=0x%.04x\n", ctx->gs);
    printk("| SS0=0x%.04x\n", ctx->ss);
    printk("| SS3=0x%.04x\n", ctx->ss3);
    printk("+------------------------------------------------------------\n");
    do_exit(1);
}

static void divide_error_exception(uint32_t esp)
{
    handle_exception("Divide Error Exception", esp);
}

static void debug_exception(uint32_t esp)
{
    handle_exception("Debug Exception", esp);
}

static void nmi_interrupt_exception(uint32_t esp)
{
    handle_exception("NMI Interrupt Exception", esp);
}

static void breakpoint_exception(uint32_t esp)
{
    handle_exception("Breakpoint Exception", esp);
}

static void overflow_exception(uint32_t esp)
{
    handle_exception("Overflow Exception", esp);
}

static void bound_range_exceeded_exception(uint32_t esp)
{
    handle_exception("Bound Range Exceeded Exception", esp);
}

static void invalid_opcode_exception(uint32_t esp)
{
    handle_exception("Invalid Opcode Exception", esp);
}

static void device_not_available_exception(uint32_t esp)
{
    handle_exception("Device Not Available Exception", esp);
}

static void coprocessor_segment_overrun_exception(uint32_t esp)
{
    handle_exception("Coprocessor Segment Overrun Exception", esp);
}

static void invalid_tss_exception(uint32_t esp)
{
    handle_exception("Invalid TSS Exception", esp);
}

static void segment_not_present_exception(uint32_t esp)
{
    handle_exception("Segment Not Present Exception", esp);
}

static void stack_segment_fault_exception(uint32_t esp)
{
    handle_exception("Stack Segment Fault Exception", esp);
}

static void general_protection_exception(uint32_t esp)
{
    handle_exception("General Protection Exception", esp);
}

static void page_fault_exception(uint32_t esp)
{
    handle_exception("Page Fault Exception", esp);
}

static void floating_point_error_exception(uint32_t esp)
{
    handle_exception("Floating-Point Error Exception", esp);
}

static void alignment_check_exception(uint32_t esp)
{
    handle_exception("Alignment Check Exception", esp);
}

static void machine_check_exception(uint32_t esp)
{
    handle_exception("Machine Check Exception", esp);
}

/*
 * Set exception handlers on standard exceptions.
 */
void init_exceptions()
{
    exception_set_handler(EXCEPT_DIVIDE_ERROR, &divide_error_exception);
    exception_set_handler(EXCEPT_DEBUG, &debug_exception);
    exception_set_handler(EXCEPT_NMI_INTERRUPT, &nmi_interrupt_exception);
    exception_set_handler(EXCEPT_BREAKPOINT, &breakpoint_exception);
    exception_set_handler(EXCEPT_OVERFLOW, &overflow_exception);
    exception_set_handler(EXCEPT_BOUND_RANGE_EXCEDEED, &bound_range_exceeded_exception);
    exception_set_handler(EXCEPT_INVALID_OPCODE, &invalid_opcode_exception);
    exception_set_handler(EXCEPT_DEVICE_NOT_AVAILABLE, &device_not_available_exception);
    exception_set_handler(EXCEPT_COPROCESSOR_SEGMENT_OVERRUN, &coprocessor_segment_overrun_exception);
    exception_set_handler(EXCEPT_INVALID_TSS, &invalid_tss_exception);
    exception_set_handler(EXCEPT_SEGMENT_NOT_PRESENT, &segment_not_present_exception);
    exception_set_handler(EXCEPT_STACK_SEGMENT_FAULT, &stack_segment_fault_exception);
    exception_set_handler(EXCEPT_GENERAL_PROTECTION, &general_protection_exception);
    exception_set_handler(EXCEPT_PAGE_FAULT, &page_fault_exception);
    exception_set_handler(EXCEPT_FLOATING_POINT_ERROR, &floating_point_error_exception);
    exception_set_handler(EXCEPT_ALIGNMENT_CHECK, &alignment_check_exception);
    exception_set_handler(EXCEPT_MACHINE_CHECK, &machine_check_exception);
}
