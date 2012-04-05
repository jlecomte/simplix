/*===========================================================================
 *
 * blockdev.c
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
 * The kernel does not want to know about the details of each individual
 * block device. Instead, the kernel code only deals with a generic block
 * device interface. Each driver, when initialized by the kernel, registers
 * itself with the block device subsystem.
 *
 *===========================================================================*/

#include <string.h>

#include <simplix/assert.h>
#include <simplix/consts.h>
#include <simplix/list.h>
#include <simplix/proto.h>
#include <simplix/types.h>

#define MAX_DESCRIPTION_LENGTH 256

/*
 * This structure represents a class of similar block devices,
 * sharing the same driver implementation.
 */
struct blkdev_class {

    /* The major number uniquely identifies a class of block devices.
       Eg: BLKDEV_IDE_MAJOR, BLKDEV_RAMDISK_MAJOR */
    unsigned int major;

    /* A short description of this class of devices.
       Eg: "IDE Hard Disk Driver" */
    char description[MAX_DESCRIPTION_LENGTH];

    /* Actual driver implementation. */
    unsigned int (* blkdev_read_impl)  (unsigned int, offset_t, unsigned int, void *);
    unsigned int (* blkdev_write_impl) (unsigned int, offset_t, unsigned int, void *);

    /* List of registered devices of this specific class. */
    struct blkdev_instance *instance_list_head;
};

/*
 * This structure represents a specific block device.
 */
struct blkdev_instance {

    /* The class this instance belongs to. */
    struct blkdev_class *class;

    /* The minor number, along with the corresponding class' major number,
       uniquely identifies a block device instance. */
    unsigned int minor;

    /* A short description of this specific instance. */
    char description[MAX_DESCRIPTION_LENGTH];

    /* The size, in bytes, of the blocks this device deals with. */
    size_t block_size;

    /* The capacity of this device in number of blocks. */
    size_t capacity;

    /* Reference counter. This instance can be safely unregistered
       only when this reference counter reaches 0. */
    unsigned int refcnt;

    /* Doubly linked list pointers. */
    struct blkdev_instance *prev, *next;
};

/* The list of block device classes. */
struct blkdev_class *blkdev_classes[NR_BLKDEV_MAJOR_TYPES] = { NULL, };

/*
 * Returns the block device instance of the specified class and minor number,
 * and increments its reference count.
 */
static struct blkdev_instance *get_blkdev_instance(struct blkdev_class *drv,
    unsigned int minor)
{
    int i;
    struct blkdev_instance *dev;
    unsigned long eflags;

    disable_hwint(eflags);

    list_for_each(drv->instance_list_head, dev, i) {
        if (dev->minor == minor) {
            dev->refcnt++;
            restore_hwint(eflags);
            return dev;
        }
    }

    restore_hwint(eflags);
    return NULL;
}

/*
 * Decrements the reference count of the specified block device instance.
 */
static void release_blkdev_instance(struct blkdev_instance *dev)
{
    unsigned long eflags;

    disable_hwint(eflags);
    ASSERT(dev->refcnt > 0);
    dev->refcnt--;
    restore_hwint(eflags);
}

/*
 * Registers a new block device class.
 */
ret_t register_blkdev_class(unsigned int major, const char *description,
    unsigned int (* blkdev_read_impl)  (unsigned int, offset_t, unsigned int, void *),
    unsigned int (* blkdev_write_impl) (unsigned int, offset_t, unsigned int, void *))
{
    struct blkdev_class *drv;
    unsigned long eflags;

    if (major > NR_BLKDEV_MAJOR_TYPES)
        return -E_INVALIDARG;

    disable_hwint(eflags);

    if (blkdev_classes[major]) {
        /* Seems like this block device class has already been registered. */
        restore_hwint(eflags);
        return -E_FAIL;
    }

    drv = __kmalloc(sizeof(struct blkdev_class));
    if (!drv) {
        restore_hwint(eflags);
        return -E_NOMEM;
    }

    drv->major = major;
    drv->blkdev_read_impl = blkdev_read_impl;
    drv->blkdev_write_impl = blkdev_write_impl;
    drv->instance_list_head = NULL;

    /* Make sure the description is null-terminated! */
    strncpy(drv->description, description, MAX_DESCRIPTION_LENGTH);
    drv->description[MAX_DESCRIPTION_LENGTH-1] = '\0';

    blkdev_classes[major] = drv;

    restore_hwint(eflags);
    return S_OK;
}

/*
 * Registers a new block device instance.
 */
ret_t register_blkdev_instance(unsigned int major, unsigned int minor,
    const char *description, size_t block_size, size_t capacity)
{
    struct blkdev_class *drv;
    struct blkdev_instance *dev;
    unsigned long eflags;

    if (major > NR_BLKDEV_MAJOR_TYPES || !block_size || !capacity)
        return -E_INVALIDARG;

    drv = blkdev_classes[major];
    if (!drv)
        return -E_INVALIDARG;

    dev = get_blkdev_instance(drv, minor);
    if (dev) {
        /* This block device instance has already been registered.
           Don't forget to decrement its reference count before returning! */
        release_blkdev_instance(dev);
        return S_OK;
    }

    dev = __kmalloc(sizeof(struct blkdev_instance));
    if (!dev)
        return -E_NOMEM;

    dev->class = drv;
    dev->minor = minor;
    dev->block_size = block_size;
    dev->capacity = capacity;
    dev->refcnt = 0;

    /* Make sure the description is null-terminated! */
    strncpy(dev->description, description, MAX_DESCRIPTION_LENGTH);
    dev->description[MAX_DESCRIPTION_LENGTH-1] = '\0';

    disable_hwint(eflags);
    list_append(drv->instance_list_head, dev);
    restore_hwint(eflags);

    return S_OK;
}

/*
 * Unregisters the specified block device instance, if it's not busy.
 */
ret_t unregister_blkdev_instance(unsigned int major, unsigned int minor)
{
    struct blkdev_class *drv;
    struct blkdev_instance *dev;
    unsigned long eflags;

    if (major > NR_BLKDEV_MAJOR_TYPES)
        return -E_INVALIDARG;

    drv = blkdev_classes[major];
    if (!drv)
        return -E_INVALIDARG;

    disable_hwint(eflags);

    dev = get_blkdev_instance(drv, minor);
    if (!dev) {
        restore_hwint(eflags);
        return -E_INVALIDARG;
    }

    /* The call to get_blkdev_instance incremented the reference count of
       this block device instance, so we need to test this value against 1.
       Note: It's important to do all this inside a critical section! */
    if (dev->refcnt > 1) {
        restore_hwint(eflags);
        return -E_BUSY;
    }

    list_remove(drv->instance_list_head, dev);
    kfree(dev);

    restore_hwint(eflags);
    return S_OK;
}

/*
 * Reads from the specified block device instance. Using an offset and/or
 * a length that don't match the corresponding device block size usually
 * comes with a severe performance penalty.
 */
ret_t blkdev_read(unsigned int major, unsigned int minor,
    loffset_t offset, size_t len, void *buffer)
{
    struct blkdev_class *drv;
    struct blkdev_instance *dev;
    size_t block_size, delta;
    unsigned int n, block, nblocks;
    byte_t *tmp, *dst = (byte_t *) buffer;

    if (major > NR_BLKDEV_MAJOR_TYPES)
        return -E_INVALIDARG;

    drv = blkdev_classes[major];
    if (!drv)
        return -E_INVALIDARG;

    dev = get_blkdev_instance(drv, minor);
    if (!dev)
        return -E_INVALIDARG;

    block_size = dev->block_size;

    /* Compute the block index and offset inside that block corresponding
       to the specified offset and block device instance. */
    block = offset / block_size;
    delta = offset % block_size;

    if (delta) {
        /* Partial read of first block. */
        tmp = __kmalloc(block_size);
        if (!tmp)
            goto error;
        if (!drv->blkdev_read_impl(minor, block, 1, tmp)) {
            kfree(tmp);
            goto error;
        }
        memcpy(dst, tmp + delta, block_size - delta);
        kfree(tmp);
        dst += block_size - delta;
        block++;
    }

    /* Compute the number of blocks to read and the offset in the last block. */
    nblocks = (offset + len) / block_size - block;
    delta = (offset + len) % block_size;

    while (nblocks) {
        /* Full read of consecutive blocks. */
        if (!(n = drv->blkdev_read_impl(minor, block, nblocks, dst)))
            goto error;
        dst += n * block_size;
        nblocks -= n;
        block += n;
    }

    if (delta) {
        /* Partial read of last block. */
        tmp = __kmalloc(block_size);
        if (!tmp)
            goto error;
        if (!drv->blkdev_read_impl(minor, block, 1, tmp)) {
            kfree(tmp);
            goto error;
        }
        memcpy(dst, tmp, delta);
        kfree(tmp);
    }

    release_blkdev_instance(dev);
    return S_OK;

error:

    release_blkdev_instance(dev);
    return -E_FAIL;
}

/*
 * Writes to the specified block device instance. Using an offset and/or
 * a length that don't match the corresponding device block size usually
 * comes with a severe performance penalty.
 */
ret_t blkdev_write(unsigned int major, unsigned int minor,
    loffset_t offset, size_t len, void *buffer)
{
    struct blkdev_class *drv;
    struct blkdev_instance *dev;
    size_t block_size, delta;
    unsigned int n, block, nblocks;
    byte_t *tmp, *src = (byte_t *) buffer;

    if (major > NR_BLKDEV_MAJOR_TYPES)
        return -E_INVALIDARG;

    drv = blkdev_classes[major];
    if (!drv)
        return -E_INVALIDARG;

    dev = get_blkdev_instance(drv, minor);
    if (!dev)
        return -E_INVALIDARG;

    block_size = dev->block_size;

    /* Compute the block index and offset inside that block corresponding
       to the specified offset and block device instance. */
    block = offset / block_size;
    delta = offset % block_size;

    if (delta) {
        /* Partial write of first block. */
        tmp = __kmalloc(block_size);
        if (!tmp)
            goto error;
        if (!drv->blkdev_read_impl(minor, block, 1, tmp)) {
            kfree(tmp);
            goto error;
        }
        memcpy(tmp + delta, src, block_size - delta);
        if (!drv->blkdev_write_impl(minor, block, 1, tmp)) {
            kfree(tmp);
            goto error;
        }
        kfree(tmp);
        src += block_size - delta;
        block++;
    }

    /* Compute the number of blocks to write and the offset in the last block. */
    nblocks = (offset + len) / block_size - block;
    delta = (offset + len) % block_size;

    while (nblocks) {
        /* Full write of consecutive blocks. */
        if (!(n = drv->blkdev_write_impl(minor, block, nblocks, src)))
            goto error;
        src += n * block_size;
        nblocks -= n;
        block += n;
    }

    if (delta) {
        /* Partial write of last block. */
        tmp = __kmalloc(block_size);
        if (!tmp)
            goto error;
        if (!drv->blkdev_read_impl(minor, block, 1, tmp)) {
            kfree(tmp);
            goto error;
        }
        memcpy(tmp, src, delta);
        if (!drv->blkdev_write_impl(minor, block, 1, tmp)) {
            kfree(tmp);
            goto error;
        }
        kfree(tmp);
    }

    release_blkdev_instance(dev);
    return S_OK;

error:

    release_blkdev_instance(dev);
    return -E_FAIL;
}
