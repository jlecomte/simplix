/*===========================================================================
 *
 * context.h
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
 * Task CPU context.
 *
 *===========================================================================*/

#ifndef _SIMPLIX_CONTEXT_H_
#define _SIMPLIX_CONTEXT_H_

#ifndef ASM_SOURCE

#include <simplix/types.h>

/* This structure represents the bottom of the stack after the corresponding
   task has been suspended. Note that the order in which the members of this
   structure are declared is absolutely crucial. It corresponds to the order
   in which the registers have been piled onto the stack in the task_switch
   function (see task_switch.S) Note: the members with a name starting with
   "__" represent the necessary padding for the corresponding field. */

struct task_cpu_context {

    /* Segment registers. */
    uint16_t ds,  __ds;
    uint16_t es,  __es;
    uint16_t fs,  __fs;
    uint16_t gs,  __gs;
    uint16_t ss,  __ss;

    /* General registers (see pusha / popa) */
    uint32_t edi, esi, ebp, esp0, ebx, edx, ecx, eax;

    /* (fake) exception error code. This value is always 0, except for those
       exceptions that have an associated error code (see consts.h) */
    uint32_t error_code;

    /* Other registers. */
    uint32_t eip;
    uint16_t cs, __cs;
    uint32_t eflags;

    /* The following values are used only for user tasks (hence the postfix
       representing the privilege level under which user tasks execute) */
    uint32_t esp3;
    uint16_t ss3, __ss3;

} __attribute__((packed));

#endif /* ASM_SOURCE */

/* Saves the current CPU context in the stack. */

#define save_context \
    pusha;           \
    push %ss;        \
    push %gs;        \
    push %fs;        \
    push %es;        \
    push %ds

/* Restores the CPU context that was saved in the stack.
   The last line takes care of the "error code" (see isr.S) */

#define restore_context \
    pop %ds;            \
    pop %es;            \
    pop %fs;            \
    pop %gs;            \
    pop %ss;            \
    popa;               \
    addl $4, %esp

#endif /* _SIMPLIX_CONTEXT_H_ */
