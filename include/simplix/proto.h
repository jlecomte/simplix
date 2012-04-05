/*===========================================================================
 *
 * proto.h
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
 * Function prototypes.
 *
 *===========================================================================*/

#ifndef _SIMPLIX_PROTO_H_
#define _SIMPLIX_PROTO_H_

#include <simplix/task.h>
#include <simplix/types.h>


/*===========================================================================*
 * blockdev.c                                                                *
 *===========================================================================*/

ret_t register_blkdev_class(unsigned int major, const char *description,
    unsigned int (* blkdev_read_impl)  (unsigned int, offset_t, unsigned int, void *),
    unsigned int (* blkdev_write_impl) (unsigned int, offset_t, unsigned int, void *));

ret_t register_blkdev_instance(unsigned int major, unsigned int minor,
    const char *description, size_t block_size, size_t capacity);

ret_t unregister_blkdev_instance(unsigned int major, unsigned int minor);

ret_t blkdev_read(unsigned int major, unsigned int minor, loffset_t offset,
    size_t len, void *buffer);

ret_t blkdev_write(unsigned int major, unsigned int minor, loffset_t offset,
    size_t len, void *buffer);


/*===========================================================================*
 * exception.c                                                               *
 *===========================================================================*/

void exception_set_handler(unsigned int numex, exception_handler_t fn);
void init_exceptions();


/*===========================================================================*
 * gdt.c                                                                     *
 *===========================================================================*/

void init_gdt(void);


/*===========================================================================*
 * gfx.c                                                                     *
 *===========================================================================*/

void init_gfx(void);
void gfx_cls(void);
void gfx_putchar(char c);
void gfx_putstring(char *s);
int gfx_get_cursor_offset(void);
void videomem_putchar(char c, int row, int col, int textattr);
void videomem_putstring(char *s, int row, int col, int textattr);


/*===========================================================================*
 * ide.c                                                                     *
 *===========================================================================*/

void init_ide_devices(void);


/*===========================================================================*
 * idt.c                                                                     *
 *===========================================================================*/

void init_idt(void);
void idt_set_handler(int index, addr_t handler_addr, unsigned int dpl);


/*===========================================================================*
 * irq.c                                                                     *
 *===========================================================================*/

void init_pic();
void enable_irq_line(int numirq);
void disable_irq_line(int numirq);
void irq_set_handler(unsigned int numirq, irq_handler_t fn);


/*===========================================================================*
 * kbd.c                                                                     *
 *===========================================================================*/

void init_kbd(void);


/*===========================================================================*
 * kmem.c                                                                    *
 *===========================================================================*/

void *kmalloc(size_t size);
void *__kmalloc(size_t size);
void kfree(void *ptr);


/*===========================================================================*
 * ksync.c                                                                   *
 *===========================================================================*/

struct ksema;
struct ksema *ksema_init(unsigned int initval);
ret_t ksema_free(struct ksema *sem);
void ksema_down(struct ksema *sem);
void ksema_up(struct ksema *sem);

struct kmutex;
struct kmutex *kmutex_init(void);
ret_t kmutex_free(struct kmutex *mut);
void kmutex_lock(struct kmutex *mut);
void kmutex_unlock(struct kmutex *mut);


/*===========================================================================*
 * main.c                                                                    *
 *===========================================================================*/

int printk(const char *format, ...);
void panic(const char *format, ...);


/*===========================================================================*
 * physmem.c                                                                 *
 *===========================================================================*/

void init_physmem(size_t memsize);
ret_t alloc_physmem_block(size_t pages, addr_t *paddr);
ret_t __alloc_physmem_block(size_t pages, addr_t *paddr);
ret_t free_physmem_block(addr_t addr);
ret_t realloc_physmem_block(addr_t addr, size_t pages, addr_t *paddr);


/*===========================================================================*
 * ramdisk.c                                                                 *
 *===========================================================================*/

void init_ramdisk_driver(void);
ret_t create_ramdisk(size_t len, unsigned int *minor);
void destroy_ramdisk(unsigned int minor);


/*===========================================================================*
 * sched.c                                                                   *
 *===========================================================================*/

pid_t alloc_pid(void);
struct task_struct *get_task(pid_t pid);
void init_multitasking(void);
void schedule(void);
void wake_up(struct task_struct *t);
void sleep_on();
void interruptible_sleep_on();


/*===========================================================================*
 * task.c                                                                    *
 *===========================================================================*/

pid_t kernel_thread(task_entry_point_t fn);
void do_exit(int status);
void do_sleep(unsigned long msec);
pid_t do_waitpid(pid_t pid, int *status);


/*===========================================================================*
 * task_switch.S                                                             *
 *===========================================================================*/

void task_switch(struct task_struct *t);


/*===========================================================================*
 * timer.c                                                                   *
 *===========================================================================*/

void init_timer(void);
void init_wall_clock(void);


#endif /* _SIMPLIX_PROTO_H_ */
