/*===========================================================================
 *
 * gdt.c
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
#include <simplix/segment.h>
#include <simplix/tss.h>
#include <simplix/types.h>

/* Our unique Task-State Segment (TSS) */
struct tss_struct tss;

/* The Global Descriptor Table (GDT) used by Simplix */
struct segment_descriptor gdt[] = {
    /* NULL descriptor */
    { 0, },
    /* Code segment descriptor */
    BUILD_4KB_SEG_DESC(0, 0xffffffff, GDT_CS_TYPE),
    /* Data segment descriptor */
    BUILD_4KB_SEG_DESC(0, 0xffffffff, GDT_DS_TYPE),
    /* Task state segment descriptor */
    { 0, },
    /* Local descriptor table (LDT) segment descriptor */
    { 0, }
};

/*
 * Initializes the Global Descriptor Table (GDT).
 */
void init_gdt(void)
{
    uint16_t tr;

    struct {
        /* Size of the GDT structure - 1 */
        uint16_t size;
        /* 32-bit linear address of the GDT */
        uint32_t addr;
    } __attribute__((__packed__)) gdtdesc;

    gdtdesc.size = sizeof(gdt) - 1;
    gdtdesc.addr = (uint32_t) gdt;

    /* Commit the GDT */
    asm("lgdt %0;"
        "ljmp %1, $1f;"
        "1: mov %2, %%ax;"
        "mov %%ax, %%ds;"
        "mov %%ax, %%es;"
        "mov %%ax, %%fs;"
        "mov %%ax, %%gs;"
        "mov %%ax, %%ss;"
        : : "m" (gdtdesc),
            "i" (GDT_CS),
            "i" (GDT_DS));

    /* Initialize the Task State Segment (TSS) descriptor and make the Task
       Register (TR) point to it. Note that the values we use in the TSS
       (esp0 and ss0) have not been initialized at this point. */
    tss.ss0 = GDT_DS;
    gdt[GDT_TSS_INDEX] = BUILD_SEG_DESC(
        (uint32_t) &tss, sizeof(tss), GDT_TSS_TYPE);
    tr = SEG_REG_VAL(KERN_PRIVILEGE_LEVEL, FALSE, GDT_TSS_INDEX);
    asm("ltr %0" : : "r" (tr));
}
