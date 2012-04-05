/*===========================================================================
 *
 * physmem.c
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
 * This file is concerned with keeping track of free and allocated segments of
 * physical memory with a granularity specified by the macro PAGE_BIT_SHIFT
 * defined in physmem.h.
 *
 *===========================================================================*/

#include <string.h>

#include <simplix/assert.h>
#include <simplix/consts.h>
#include <simplix/list.h>
#include <simplix/macros.h>
#include <simplix/proto.h>
#include <simplix/types.h>

/*
 * This structure represents an available block of contiguous physical memory.
 */
struct block {

    /* Number of pages in this block */
    unsigned int pages;

    /* Flag indicating whether this block is a available or allocated. */
    bool_t available;

    /* Doubly linked list pointers. */
    struct block *prev, *next;
};

/* Position of the BIOS and Video memory in physical memory on a PC */
#define BIOS_AND_VIDEO_MEMORY_START 0x0a0000
#define BIOS_AND_VIDEO_MEMORY_END   0x100000

/* Block descriptor array */
static struct block *first_block_descriptor = NULL;

/* List of blocks (allocated or not) in physical memory */
static struct block *block_list_head = NULL;

/* Size of the physical memory. */
size_t physmem_size;

/*
 * Returns the block descriptor corresponding to the specified address.
 * Note: There is no check on the validity of the specified address.
 */
#define get_block_descriptor(addr) \
    (first_block_descriptor + ((addr) >> PAGE_BIT_SHIFT));

/*
 * Returns the base address corresponding to the specified block descriptor.
 */
#define get_block_descriptor_addr(b) \
    (((b) - first_block_descriptor) << PAGE_BIT_SHIFT);

/*
 * Initializes the physical memory management module using the specified
 * physical memory size (sent to the kernel by our boot loader)
 */
void init_physmem(size_t memsize)
{
    int count;
    struct block *b;
    bool_t use_low_mem = TRUE;
    addr_t descriptor_array_end_addr;

    /* Physical memory address of the end of the kernel (see simplix.lds) */
    extern char __e_kernel;

    /* The maximum number of physical memory block descriptors needed is the
       number of pages in the specified amount of physical memory. */

    count = memsize >> PAGE_BIT_SHIFT;

    /* Round the size of the physical memory to an exact number of pages. */

    physmem_size = count << PAGE_BIT_SHIFT;

    /* Are we going to store the array of physical memory block descriptors in
       conventional memory or in extended memory? */

    if (count * sizeof(struct block) <= BIOS_AND_VIDEO_MEMORY_START - (addr_t) &__e_kernel) {
        first_block_descriptor = (struct block *) &__e_kernel;
    } else if (count * sizeof(struct block) <= memsize - BIOS_AND_VIDEO_MEMORY_END) {
        first_block_descriptor = (struct block *) BIOS_AND_VIDEO_MEMORY_END;
        use_low_mem = FALSE;
    } else {
        panic("Initialization of physical memory subsystem failed.");
    }

    descriptor_array_end_addr = (addr_t) (first_block_descriptor + count);

    /* Initialize the list of blocks in physical memory. */

    #define add_block(start_addr, end_addr, is_hole)           \
        b = get_block_descriptor(PAGE_ALIGN_INF(start_addr));  \
        b->pages = (PAGE_ALIGN_SUP(end_addr) -                 \
            PAGE_ALIGN_INF(start_addr)) >> PAGE_BIT_SHIFT;     \
        b->available = (is_hole);                              \
        list_append(block_list_head, b);

    if (use_low_mem) {
        add_block(0, descriptor_array_end_addr, FALSE);
        add_block(descriptor_array_end_addr, BIOS_AND_VIDEO_MEMORY_START, TRUE);
        add_block(BIOS_AND_VIDEO_MEMORY_START, BIOS_AND_VIDEO_MEMORY_END, FALSE);
        add_block(BIOS_AND_VIDEO_MEMORY_END, memsize, TRUE);
    } else {
        add_block(0, (addr_t) &__e_kernel, FALSE);
        add_block((addr_t) &__e_kernel, BIOS_AND_VIDEO_MEMORY_START, TRUE);
        add_block(BIOS_AND_VIDEO_MEMORY_START, descriptor_array_end_addr, FALSE);
        add_block(descriptor_array_end_addr, memsize, TRUE);
    }
}

/*
 * Same as alloc_physmem_block, but does not zero-out the allocated block of
 * memory.
 */
ret_t __alloc_physmem_block(size_t pages, addr_t *paddr)
{
    int i;
    addr_t addr;
    struct block *b, *h;
    unsigned long eflags;

    if (!pages || !paddr)
        return -E_INVALIDARG;

    disable_hwint(eflags);

    /* Find a hole big enough to accomodate the specified number of pages
       using the first-fit algorithm. */
    list_for_each(block_list_head, b, i)
        if (b->available && b->pages >= pages)
            goto found;

    /* No big enough hole was found to fit the specified number of pages. */
    restore_hwint(eflags);
    return -E_NOMEM;

found:

    b->available = FALSE;
    addr = get_block_descriptor_addr(b);
    if (b->pages > pages) {
        /* A new hole has to be inserted after this block. */
        h = get_block_descriptor(addr + (pages << PAGE_BIT_SHIFT));
        h->available = TRUE;
        h->pages = b->pages - pages;
        b->pages = pages;
        list_insert_after(b, h);
    }

    *paddr = addr;
    restore_hwint(eflags);
    return S_OK;
}

/*
 * Allocates a block of physical memory containing the specified number of
 * pages. Returns the base address of the newly allocated block in the
 * specified paddr parameter. This address is aligned on a page. The
 * allocated block of memory is filled with zeros.
 */
ret_t alloc_physmem_block(size_t pages, addr_t *paddr)
{
    ret_t res = __alloc_physmem_block(pages, paddr);
    if (res == S_OK)
        memset((void *) *paddr, 0, pages << PAGE_BIT_SHIFT);
    return res;
}

/*
 * Frees the block of physical memory starting at the specified address.
 */
ret_t free_physmem_block(addr_t addr)
{
    struct block *b;
    unsigned long eflags;

    if (addr >= physmem_size - PAGE_SIZE)
        panic("Trying to free an invalid block of physical memory");

    disable_hwint(eflags);

    b = get_block_descriptor(addr);
    if (b->available) {
        restore_hwint(eflags);
        return -E_FAIL;
    }

    b->available = TRUE;

    if (b < b->next && b->next->available) {
        /* This hole is followed by another hole. Merge the two holes. */
        b->pages += b->next->pages;
        list_remove(block_list_head, b->next);
    }

    if (b->prev < b && b->prev->available) {
        /* This hole is preceded by another hole. Merge the two holes. */
        b->prev->pages += b->pages;
        list_remove(block_list_head, b);
    }

    restore_hwint(eflags);
    return S_OK;
}

/*
 * Changes the size of the block of physical memory starting at address addr to
 * pages physical memory pages. The block may need to be relocated, and the new
 * address can be retrieved in the paddr parameter. Newly allocated memory is
 * filled with zeros.
 */
ret_t realloc_physmem_block(addr_t addr, size_t pages, addr_t *paddr)
{
    ret_t res;
    struct block *b, *h;
    unsigned long eflags;

    if (!pages || !paddr)
        return -E_INVALIDARG;

    disable_hwint(eflags);

    b = get_block_descriptor(addr);
    if (b->available) {
        restore_hwint(eflags);
        return -E_FAIL;
    }

    if (pages == b->pages) {

        /* No change needed. */
        *paddr = addr;
        restore_hwint(eflags);
        return S_OK;

    } else if (pages < b->pages) {

        /* Shrink the block. This is always a cheap operation. */

        if (b < b->next && b->next->available) {
            /* This block is followed by a hole. Widen the following hole. */
            h = get_block_descriptor(addr + (pages << PAGE_BIT_SHIFT));
            h->available = TRUE;
            h->pages = b->next->pages + b->pages - pages;
            list_replace(block_list_head, b->next, h);
        } else {
            /* Either this block was the last block in memory, or it is followed
               by an allocated block. Insert a new hole right after it. */
            h = get_block_descriptor(addr + (pages << PAGE_BIT_SHIFT));
            h->available = TRUE;
            h->pages = b->pages - pages;
            list_insert_after(b, h);
        }

        /* Shrink the block. If everybody else does their job right, it should
           be safe not to erase the content of the deallocated memory. */
        b->pages = pages;
        *paddr = addr;

    } else {

        /* Widen the block. */

        if (b < b->next && b->next->available && b->next->pages >= pages - b->pages) {
            /* This block is followed by a hole wide enough. */

            if (b->next->pages > pages - b->pages) {
                /* Shrink the following hole. */
                h = get_block_descriptor(addr + (pages << PAGE_BIT_SHIFT));
                h->available = TRUE;
                h->pages = b->next->pages - pages + b->pages;
                list_replace(block_list_head, b->next, h);
            } else {
                /* Remove the following hole. */
                list_remove(block_list_head, h);
            }

            /* Widen the block. */
            b->pages = pages;
            *paddr = addr;

        } else {

            /* The block needs to be reallocated. This is expensive. */
            res = __alloc_physmem_block(pages, paddr);
            if (res != S_OK) {
                restore_hwint(eflags);
                return res;
            }
            /* Copy the old data into the newly allocated block. */
            memcpy((void *) *paddr, (void *) addr, b->pages << PAGE_BIT_SHIFT);
            /* Fill the remaining memory area with zeros. */
            memset((void *) (*paddr + (b->pages << PAGE_BIT_SHIFT)), 0,
                (pages - b->pages) << PAGE_BIT_SHIFT);
            /* And free the old block. */
            res = free_physmem_block(addr);
            ASSERT(res == S_OK);

        }

    }

    restore_hwint(eflags);
    return S_OK;
}
