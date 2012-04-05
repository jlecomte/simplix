/*===========================================================================
 *
 * kmem.c
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
 * This is a high performance memory cache for the Simplix kernel.
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
 * This structure represents a block of physical memory. It is 12 bytes long on
 * 32 bit architectures, which is fairly large, especially compared to some of
 * the smaller objects this allocator deals with. However, memory is cheap
 * nowadays, and CPU time is always a precious commodity, so it's fine.
 */
struct kmem_object {

    /* The cache this block belongs to. */
    struct kmem_cache *cache;

    /* Doubly linked list pointers. */
    struct kmem_object *prev, *next;
};

/*
 * This structure represents a block of contiguous physical memory allocated by
 * the physical memory manager. All the smaller objects this allocator deals
 * with are contained within such blocks.
 */
struct kmem_cache {

    /* The slab this cache belongs to. */
    struct kmem_slab *slab;

    /* Number of unallocated objects in this cache. */
    unsigned int nr_free_objects;

    /* List of free objects in this cache. */
    struct kmem_object *free_object_list_head;

    /* Doubly linked list pointers. */
    struct kmem_cache *prev, *next;
};

/*
 * This structure represents a group of caches for similar size objects.
 */
struct kmem_slab {

    /* List of caches within this slab. */
    struct kmem_cache *cache_list_head;
};

/*
 * Caches have a static size. This means that the list of caches in slabs
 * containing large objects can be quite long. However, by making sure that the
 * cache at the beginning of that list always contains at least one unallocated
 * object, this design decision does not impact allocation performance.
 */
#define NR_PAGES_PER_KMEM_CACHE 8

/* Expression of the granularity of this allocator in bytes (1 << 3 = 8) */
#define KMEM_CACHE_GRANULARITY 3

/* Number of entries in the slabs array. */
#define KMEM_SLAB_ARRAY_SIZE 128

/* Minimum/maximum object size. */
#define KMEM_CACHE_MIN_OBJ_SIZE (1 << KMEM_CACHE_GRANULARITY)
#define KMEM_CACHE_MAX_OBJ_SIZE (KMEM_SLAB_ARRAY_SIZE << KMEM_CACHE_GRANULARITY)

/*
 * Array of slabs, indexed by the size of the objects they contain:
 *
 *     [0] ->    8B objects
 *     [1] ->   16B objects
 *     [2] ->   24B objects
 *     [3] ->   32B objects
 *     [4] ->   40B objects
 *     [5] ->   48B objects
 *     ...
 *   [127] -> 1024B objects
 */
static struct kmem_slab slabs[KMEM_SLAB_ARRAY_SIZE] = {
    { NULL },
};

/*
 * Same as kmalloc, but does not zero-out the allocated block of memory.
 */
void *__kmalloc(size_t size)
{
    int idx;
    size_t cache_size, object_size;
    addr_t cache_addr, object_addr;
    struct kmem_slab *slab;
    struct kmem_cache *cache;
    struct kmem_object *object;
    unsigned long eflags;

    ASSERT(size > 0 && size < KMEM_CACHE_MAX_OBJ_SIZE);

    disable_hwint(eflags);

    /* Find the slab corresponding to the specified size. */
    idx = (size - 1) >> KMEM_CACHE_GRANULARITY;
    slab = &slabs[idx];

    /* Look in the cache at the beginning of the slab for a free object. */
    cache = slab->cache_list_head;

    if (!cache || !cache->nr_free_objects) {

        /* Either this slab has not yet been initialized, or all the
           caches in this slab are full. Let's allocate a new cache. */
        if (alloc_physmem_block(NR_PAGES_PER_KMEM_CACHE, &cache_addr) != S_OK) {
            restore_hwint(eflags);
            return NULL;
        }

        /* Compute the actual size of the slots in that cache. */
        size = idx << KMEM_CACHE_GRANULARITY;

        /* Initialize the newly created cache. */
        cache = (struct kmem_cache *) cache_addr;
        cache->slab = slab;
        cache_size = NR_PAGES_PER_KMEM_CACHE << PAGE_BIT_SHIFT;
        object_size = sizeof(struct kmem_object) + size;
        cache->nr_free_objects = (cache_size - sizeof(struct kmem_cache)) / object_size;

        /* Initialize the list of unallocated objects in the newly created cache. */
        for (object_addr = cache_addr + sizeof(struct kmem_cache);
             object_addr < cache_addr + cache_size;
             object_addr += object_size) {
            object = (struct kmem_object *) object_addr;
            object->cache = cache;
            list_append(cache->free_object_list_head, object);
        }

        /* Prepend the newly created cache to the corresponding slab. */
        list_append(slab->cache_list_head, cache);
        slab->cache_list_head = cache;
    }

    /* Remove the object from the cache, and return its address. */
    cache->nr_free_objects--;
    object = list_pop_head(cache->free_object_list_head);
    restore_hwint(eflags);
    return (void *) ((addr_t) object + sizeof(struct kmem_object));
}

/*
 * Allocates a block of physical memory of the specified size and returns
 * its address. The allocated block of memory is filled with zeros.
 */
void *kmalloc(size_t size)
{
    void *ptr = __kmalloc(size);
    if (ptr)
        memset(ptr, 0, size);
    return ptr;
}

/*
 * Frees the block of physical memory allocated using kmalloc or __kmalloc
 * and located at the specified address.
 */
void kfree(void *ptr)
{
    struct kmem_object *object;
    struct kmem_cache *cache;
    struct kmem_slab *slab;
    unsigned long eflags;

    disable_hwint(eflags);

    /* Find out which cache and slab this object belongs to. */
    object = (struct kmem_object *) ((addr_t) ptr - sizeof(struct kmem_object));
    cache = object->cache;
    slab = cache->slab;

    /* Return the object to the cache. */
    list_append(cache->free_object_list_head, object);
    cache->nr_free_objects++;

    /* Place this cache at the beginning of the slab. */
    slab->cache_list_head = cache;

    restore_hwint(eflags);
}
