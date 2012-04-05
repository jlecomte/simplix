/*===========================================================================
 *
 * irq.c
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
 * This file contains a set of routines used to control the Programmable
 * Interrupt Controller (PIC, model 8259A), the hardware interrupts and
 * their associated handling routines.
 *
 *===========================================================================*/

#include <simplix/assert.h>
#include <simplix/consts.h>
#include <simplix/io.h>
#include <simplix/proto.h>
#include <simplix/types.h>

/* The offset IRQs are remapped when the system starts.
   Note: This value MUST be divisible by 8! */
#define IRQ_OFFSET 0x20

/* Array of ISRs (defined in isr.S) */
extern addr_t irq_wrapper_array[NR_IRQS];

/* Array of interrupt handlers */
irq_handler_t irq_handler_array[NR_IRQS] = { NULL, };

/*
 * When the system starts, the BIOS configures the two PICs and maps IRQs
 * 0..7, 0x8..0xf to INTs 0x8..0xf, 0x70..0x77. However, in protected mode,
 * INTs 0..0x1f are reserved for exceptions. This means IRQs need to be
 * remapped. This is done by controlling the two PICs.
 */
void init_pic()
{
    /* Send ICW1: start the initialization sequence + ICW4 needed */
    outb(PIC1_CMD, 0x11); udelay(1);
    outb(PIC2_CMD, 0x11); udelay(1);

    /* Send ICW2: ctrl base address */
    outb(PIC1_DATA, IRQ_OFFSET); udelay(1);
    outb(PIC2_DATA, IRQ_OFFSET + 8); udelay(1);

    /* Send ICW3 master: mask where slave is connected.
       Send ICW3 slave: index where the slave is connected on master. */
    outb(PIC1_DATA, 0x4); udelay(1);
    outb(PIC2_DATA, 0x2); udelay(1);

    /* Send ICW4: 8086 mode, fully nested, not buffered, no implicit EOI. */
    outb(PIC1_DATA, 0x1); udelay(1);
    outb(PIC2_DATA, 0x1); udelay(1);

    /* Send OCW1: Disable all IRQs. The only IRQ enabled is the cascade:
       Since the slave is connected to pin #2 on the master, we pass 0xfb
       to the master. */
    outb(PIC1_DATA, 0xfb); udelay(1);
    outb(PIC2_DATA, 0xff); udelay(1);
}

/*
 * Enable the specified IRQ line.
 */
void enable_irq_line(int numirq)
{
    if (numirq < 8) {
        /* irq on master PIC. */
        outb(PIC1_DATA, inb(PIC1_DATA) & ~(1 << numirq));
    } else {
        /* irq on slave PIC. */
        outb(PIC2_DATA, inb(PIC2_DATA) & ~(1 << (numirq - 8)));
    }
}

/*
 * Disable the specified IRQ line.
 */
void disable_irq_line(int numirq)
{
    if (numirq < 8) {
        /* irq on master PIC. */
        outb(PIC1_DATA, inb(PIC1_DATA) | (1 << numirq));
    } else {
        /* irq on slave PIC. */
        outb(PIC2_DATA, inb(PIC2_DATA) | (1 << (numirq - 8)));
    }
}

/*
 * Set/Unset standard interrupt handlers.
 */
void irq_set_handler(unsigned int numirq, irq_handler_t fn)
{
    addr_t isr;

    ASSERT(numirq < NR_IRQS);

    irq_handler_array[numirq] = fn;

    isr = fn == NULL ? (addr_t) NULL : irq_wrapper_array[numirq];
    idt_set_handler(IRQ_OFFSET + numirq, isr, KERN_PRIVILEGE_LEVEL);
}
