/*===========================================================================
 *
 * tss.h
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
 * x86 Task-State Segment (TSS)
 * See Intel Developer's Manual Volume 3 - section 6.2.1
 *
 *===========================================================================*/

#ifndef _SIMPLIX_TSS_H_
#define _SIMPLIX_TSS_H_

#include <simplix/types.h>

/* Note: the members with a name starting with "__" represent
   the necessary padding for the corresponding field. */

struct tss_struct {

    /* Segment selector for the TSS of the previous task. */
    uint16_t back_link, __bsh;

    /* Privilege level-0, -1, and -2 stack pointer fields. */
    uint32_t esp0;
    uint16_t ss0, __ss0h;
    uint32_t esp1;
    uint16_t ss1, __ss1h;
    uint32_t esp2;
    uint16_t ss2, __ss2h;

    /* CR3 control register field (aka PDBR) */
    uint32_t cr3;

    /* EIP (instruction pointer) field. */
    uint32_t eip;

    /* EFLAGS register field. */
    uint32_t eflags;

    /* General purpose registers. */
    uint32_t eax, ecx, edx, ebx, esp, ebp, esi, edi;

    /* Segment selector fields. */
    uint16_t es, __esh;
    uint16_t cs, __csh;
    uint16_t ss, __ssh;
    uint16_t ds, __dsh;
    uint16_t fs, __fsh;
    uint16_t gs, __gsh;

    /* LDT segment selector field. */
    uint16_t ldt, __ldth;

    /* T (debug trap) flag (byte 100, bit 0) */
    uint16_t t:1, __t:15;

    /* I/O map base address field. */
    uint16_t io_map_base_addr;

} __attribute__((packed));

#endif /* _SIMPLIX_TSS_H_ */
