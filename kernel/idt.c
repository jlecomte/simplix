/*===========================================================================
 *
 * idt.c
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
 * This file contains a set of routines used to control the Interrupt Descriptor
 * Table, or IDT.
 *
 * See Intel Developer's Manual Volume 3 - section 5.9
 *
 *===========================================================================*/

#include <string.h>

#include <simplix/assert.h>
#include <simplix/consts.h>
#include <simplix/proto.h>
#include <simplix/segment.h>
#include <simplix/types.h>

#define IDT_ENTRY_COUNT 256

#define IDT_TASK_GATE_TYPE 0x5
#define IDT_INT_GATE_TYPE  0xe
#define IDT_CALL_GATE_TYPE 0xf

struct idt_descriptor {

    /* First 16 bits of the offset to procedure entry point in the segment. */
    uint16_t isr_offset_15_0;

    /* Segment selector for destination code segment. */
    uint16_t cs;
    uint8_t zero;

    /* Gate type
       0x5 for a task gate
       0xe for an interrupt gate
       0xf for a call gate */
    uint8_t type:5;

    /* Lowest privilege level this gate can be accessed from.
       See Intel Developer's Manual Volume 3 - section 5.10.1.1 */
    uint8_t dpl:2;

    /* Present bit */
    uint8_t p:1;

    /* Bits 31-16 of the offset to procedure entry point in the segment. */
    uint16_t isr_offset_31_16;

} __attribute__((__packed__));

static struct idt_descriptor idt[IDT_ENTRY_COUNT];

#define BUILD_IDT_DESCRIPTOR(addr, present, privilege, gatetype) \
    ((struct idt_descriptor) {                                   \
        .isr_offset_15_0 = (addr) & 0xffff,                      \
        .isr_offset_31_16 = ((addr) >> 16) & 0xffff,             \
        .p = (present),                                          \
        .dpl = (privilege),                                      \
        .type = (gatetype),                                      \
        .cs = GDT_CS,                                            \
        .zero = 0                                                \
    })

/*
 * Initializes the Interrupt Descriptor Table (IDT).
 */
void init_idt(void)
{
    struct idt_desc {

        /* Size of the IDT structure - 1 */
        uint16_t size;

        /* 32-bit linear address of the IDT. */
        uint32_t addr;

    } __attribute__((__packed__)) idtdesc;

    /* Initialize the IDT */
    memset(idt, 0, sizeof(idt));

    /* Commit the IDT */
    idtdesc.size = sizeof(idt) - 1;
    idtdesc.addr = (uint32_t) idt;
    asm volatile("lidt %0" : : "m" (idtdesc) : "memory");
}

/*
 * Attaches the specified handler to the specified interrupt level. If the
 * specified handler is NULL, it detaches any previously attached handler
 * to the specified interrupt level.
 */
void idt_set_handler(int index, addr_t handler_addr, unsigned int dpl)
{
    ASSERT(dpl <= USER_PRIVILEGE_LEVEL);

    if (handler_addr == (addr_t) NULL) {
        memset(idt + index, 0, sizeof(struct idt_descriptor));
    } else {
        idt[index] = BUILD_IDT_DESCRIPTOR(handler_addr, TRUE, dpl,
            IDT_INT_GATE_TYPE);
    }
}
