/*===========================================================================
 *
 * segment.h
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
 * x86 segment selectors and segment descriptors
 * See Intel Developer's Manual Volume 3 - sections 3.4.1 and 3.4.3
 *
 *===========================================================================*/

#ifndef _SIMPLIX_SEGMENT_H_
#define _SIMPLIX_SEGMENT_H_

#ifndef ASM_SOURCE

#include <simplix/consts.h>
#include <simplix/types.h>

struct segment_descriptor {

    /* First 16 bits in the segment limiter. */
    uint16_t limit_15_0;

    /* First 16 bits in the base address. */
    uint16_t base_addr_15_0;

    /* Bits 16-23 in the base address. */
    uint8_t base_addr_23_16;

    /* Since all descriptors do not have exactly the same structure (e.g. a TSS
       descriptor has a slightly different structure from an LDT descriptor),
       we group several fields into the "type" member. */
    uint8_t type;

    /* Bits 16-19 in the segment limiter. */
    uint8_t limit_19_16:4;

    /* User (OS) defined bit. */
    uint8_t u:1;

    /* Reserved by Intel. */
    uint8_t x:1;

    /*  Indicates how the instructions (80386 and up) access register and memory
        data in protected mode. When D=0, instructions are 16-bit instructions,
        with 16-bit offsets and 16-bit registers. Stacks are assumed 16-bit wide
        and SP is used. When D=1, 32-bits are assumed. */
    uint8_t d:1;

    /* When G=0, segments can be 1 byte to 1MB in length.
       When G=1, segments can be 4KB to 4GB in length. */
    uint8_t g:1;

    /* The last 24-31 bits in the base address. */
    uint8_t base_addr_31_24;

} __attribute__((__packed__));

/* Build a byte granular segment descriptor. */
#define BUILD_SEG_DESC(base_addr, limit, segtype)      \
    ((struct segment_descriptor) {                     \
        .limit_15_0 = (limit) & 0xffff,                \
        .limit_19_16 = ((limit) >> 16) & 0xf,          \
        .base_addr_15_0 = (base_addr) & 0xffff,        \
        .base_addr_23_16 = ((base_addr) >> 16) & 0xff, \
        .base_addr_31_24 = ((base_addr) >> 24) & 0xff, \
        .u = 0, .x = 0, .d = 1, .g = 0,                \
        .type = (segtype)                              \
    })

/* Build a 4KB granular segment descriptor. */
#define BUILD_4KB_SEG_DESC(base_addr, limit, segtype)  \
    ((struct segment_descriptor) {                     \
        .limit_15_0 = ((limit) >> 12) & 0xffff,        \
        .limit_19_16 = ((limit) >> 28) & 0xf,          \
        .base_addr_15_0 = (base_addr) & 0xffff,        \
        .base_addr_23_16 = ((base_addr) >> 16) & 0xff, \
        .base_addr_31_24 = ((base_addr) >> 24) & 0xff, \
        .u = 0, .x = 0, .d = 1, .g = 1,                \
        .type = (segtype)                              \
    })

/* Returns the base address of a segment. */
#define SEG_ADDR(seg_desc)                 \
    (((seg_desc)->base_addr_31_24 << 24) | \
     ((seg_desc)->base_addr_23_16 << 16) | \
     (seg_desc)->base_addr_15_0)

/* Returns the limit of a segment. */
#define SEG_SIZE(seg_desc)                                                \
    ((seg_desc)->g ?                                                      \
     ((((seg_desc)->limit_19_16 << 16) | (seg_desc)->limit_15_0) << 12) : \
     (((seg_desc)->limit_19_16 << 16) | (seg_desc)->limit_15_0))

#endif /* ASM_SOURCE */

/* Returns the value of the segment selector associated with the specified
   segment properties. */
#define SEG_REG_VAL(privilege, in_ldt, segment_index) \
    (((segment_index) << 3) | ((in_ldt) << 2) | (privilege & 0x3))

/* Returns the Request Privilege Level of the specified segment selector. */
#define SEG_REG_RPL(seg_reg_val) ((seg_reg_val) & 0x3)

/* Computes the physical memory address from the virtual memory address. */
#define GET_PHYSMEM_ADDR(vaddr) \
    ((vaddr) + SEG_ADDR(&current->ldt[LDT_DS_INDEX]))

/* Verifies that a virtual memory area is valid. */
#define VALIDATE_VMEM_AREA(vaddr, size) \
    ((vaddr) < SEG_SIZE(&current->ldt[LDT_DS_INDEX]) - (size))

/* Prior to an interrupt or an exception (including a system call), we don't
   know what the privilege level was, but we know we are now officially in
   kernel mode! Therefore, we need the appropriate value in the data segment
   registers (we may be coming from user mode, which does not use system
   segments) Note that although this macro uses the EAX register, it restores
   it to its previous value using the stack, but without changing the value of
   the stack pointer. */

#define restore_system_segments \
    push %eax;                  \
    mov $GDT_DS, %ax;           \
    mov %ax, %ds;               \
    mov %ax, %es;               \
    mov %ax, %fs;               \
    mov %ax, %gs;               \
    pop %eax;

#endif /* _SIMPLIX_SEGMENT_H_ */
