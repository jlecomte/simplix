/*===========================================================================
 *
 * ramdisk.c
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
 * RAM Disk driver.
 *
 *===========================================================================*/

#include <string.h>

#include <simplix/consts.h>
#include <simplix/list.h>
#include <simplix/proto.h>
#include <simplix/types.h>

/* The block size, in bytes. */
#define BLOCK_SIZE  512

struct ramdisk {

    /* The minor number associated with this RAM disk instance. */
    unsigned int minor;

    /* The physical memory address at which this RAM disk is located. */
    addr_t addr;

    /* The capacity of this RAM disk instance, in number of blocks. */
    unsigned int nblocks;

    /* Doubly linked list pointers. */
    struct ramdisk *prev, *next;
};

struct ramdisk *ramdisk_list_head = NULL;

/*
 * Returns the RAM disk instance associated with the specified minor number.
 */
static struct ramdisk *get_ramdisk_instance(unsigned int minor)
{
    int i;
    struct ramdisk *rd;
    unsigned long eflags;

    disable_hwint(eflags);

    /* Look for the RAM disk instance with the specified minor number. */
    list_for_each(ramdisk_list_head, rd, i) {
        if (rd->minor == minor) {
            restore_hwint(eflags);
            return rd;
        }
    }

    restore_hwint(eflags);
    return NULL;
}

/*
 * Generic read/write function.
 */
static unsigned int ramdisk_read_write_blocks(unsigned int minor,
    offset_t block, unsigned int nblocks, void *buffer, bool_t w)
{
    struct ramdisk *rd;
    addr_t addr;
    size_t size;

    rd = get_ramdisk_instance(minor);
    if (!rd || block + nblocks > rd->nblocks)
        return 0;

    addr = rd->addr + block * BLOCK_SIZE;
    size = nblocks * BLOCK_SIZE;

    if (w) {
        memcpy((void *) addr, buffer, size);
    } else {
        memcpy(buffer, (void *) addr, size);
    }

    return nblocks;
}

/*
 * Read the specified block from the specified RAM disk, and copy its content to
 * the destination buffer. The work is delegated to ramdisk_read_write_blocks.
 */
static unsigned int ramdisk_read_blocks(unsigned int minor, offset_t block,
    unsigned int nblocks, void *buffer)
{
    return ramdisk_read_write_blocks(minor, block, nblocks, buffer, FALSE);
}

/*
 * Write the content of the source buffer in the specified block of the
 * specified RAM disk. The work is delegated to ramdisk_read_write_blocks.
 */
static unsigned int ramdisk_write_blocks(unsigned int minor, offset_t block,
    unsigned int nblocks, void *buffer)
{
    return ramdisk_read_write_blocks(minor, block, nblocks, buffer, TRUE);
}

/*
 * Initialize the RAM disk driver.
 */
void init_ramdisk_driver(void)
{
    register_blkdev_class(BLKDEV_RAM_DISK_MAJOR, "RAM Disk Driver",
        &ramdisk_read_blocks, &ramdisk_write_blocks);
}

/*
 * Creates a new RAM disk instance that's at least len bytes.
 */
ret_t create_ramdisk(size_t len, unsigned int *minor)
{
    static unsigned int n = 0;
    struct ramdisk *rd;
    unsigned int pages;
    unsigned long eflags;

    if (!len)
        return -E_INVALIDARG;

    rd = __kmalloc(sizeof(struct ramdisk));
    if (!rd)
        return -E_NOMEM;

    pages = PAGE_ALIGN_SUP(len) >> PAGE_BIT_SHIFT;
    if (alloc_physmem_block(pages, &rd->addr) != S_OK) {
        kfree(rd);
        return -E_NOMEM;
    }

    rd->nblocks = (pages << PAGE_BIT_SHIFT) / BLOCK_SIZE;

    disable_hwint(eflags);
    rd->minor = n++;
    list_append(ramdisk_list_head, rd);
    restore_hwint(eflags);

    *minor = rd->minor;
    return S_OK;
}

/*
 * Destroys the specified RAM disk.
 */
void destroy_ramdisk(unsigned int minor)
{
    struct ramdisk *rd;
    unsigned long eflags;

    disable_hwint(eflags);

    rd = get_ramdisk_instance(minor);
    if (!rd) {
        restore_hwint(eflags);
        return;
    }

    free_physmem_block(rd->addr);
    list_remove(ramdisk_list_head, rd);
    kfree(rd);
    restore_hwint(eflags);
}
