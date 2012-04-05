/*===========================================================================
 *
 * consts.h
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
 * Constants.
 *
 *===========================================================================*/

#ifndef _SIMPLIX_CONST_H_
#define _SIMPLIX_CONST_H_

#include <simplix/macros.h>

/*===========================================================================*
 * Generic error codes, used throughout the kernel.                          *
 *===========================================================================*/

/* Generic success code. */
#define S_OK 0

/* Generic error code. */
#define E_FAIL 1

/* One or several arguments are invalid. */
#define E_INVALIDARG 2

/* Memory allocation failed. */
#define E_NOMEM 3

/* Unknown system call number. */
#define E_NOSYS 4

/* Device busy. */
#define E_BUSY 5


/*===========================================================================*
 * Constants related to the interrupt controller.                            *
 *===========================================================================*/

#define PIC1_CMD   0x20  /* IO base address for master PIC */
#define PIC2_CMD   0xa0  /* IO base address for slave PIC */
#define PIC1_DATA  0x21  /* Master PIC data port */
#define PIC2_DATA  0xa1  /* Slave PIC data port */
#define PIC_EOI    0x20  /* End - of - interrupt command code */

/* Number of standard IRQ levels on a PC. */
#define NR_IRQS 16

/* List of standard IRQ levels on a PC. Keep in mind that pin #2 is connected
   to the slave PIC, and that pin #9 is connected to the master PIC. */
#define IRQ_TIMER            0
#define IRQ_KEYBOARD         1
#define SLAVE_PIC            2
#define IRQ_COM2             3
#define IRQ_COM1             4
#define IRQ_LPT2             5
#define IRQ_FLOPPY           6
#define IRQ_LPT1             7
#define IRQ_RT_CLOCK         8
#define MASTER_PIC           9
#define IRQ_AVAILABLE_1     10
#define IRQ_AVAILABLE_2     11
#define IRQ_PS2_MOUSE       12
#define IRQ_COPROCESSOR     13
#define IRQ_PRIMARY_IDE     14
#define IRQ_SECONDARY_IDE   15


/*===========================================================================*
 * Constants related to software interrupts a.k.a. exceptions.               *
 *===========================================================================*/

/* Number of standard exceptions on a PC. */
#define NR_EXCEPTIONS 32

/* List of standard Intel x86 exceptions.
   See Intel x86 doc vol 3, section 5.12. */
#define EXCEPT_DIVIDE_ERROR                  0         // No error code
#define EXCEPT_DEBUG                         1         // No error code
#define EXCEPT_NMI_INTERRUPT                 2         // No error code
#define EXCEPT_BREAKPOINT                    3         // No error code
#define EXCEPT_OVERFLOW                      4         // No error code
#define EXCEPT_BOUND_RANGE_EXCEDEED          5         // No error code
#define EXCEPT_INVALID_OPCODE                6         // No error code
#define EXCEPT_DEVICE_NOT_AVAILABLE          7         // No error code
#define EXCEPT_DOUBLE_FAULT                  8         // Yes (Zero)
#define EXCEPT_COPROCESSOR_SEGMENT_OVERRUN   9         // No error code
#define EXCEPT_INVALID_TSS                  10         // Yes
#define EXCEPT_SEGMENT_NOT_PRESENT          11         // Yes
#define EXCEPT_STACK_SEGMENT_FAULT          12         // Yes
#define EXCEPT_GENERAL_PROTECTION           13         // Yes
#define EXCEPT_PAGE_FAULT                   14         // Yes
#define EXCEPT_INTEL_RESERVED_1             15         // No
#define EXCEPT_FLOATING_POINT_ERROR         16         // No
#define EXCEPT_ALIGNMENT_CHECK              17         // Yes (Zero)
#define EXCEPT_MACHINE_CHECK                18         // No
#define EXCEPT_INTEL_RESERVED_2             19         // No
#define EXCEPT_INTEL_RESERVED_3             20         // No
#define EXCEPT_INTEL_RESERVED_4             21         // No
#define EXCEPT_INTEL_RESERVED_5             22         // No
#define EXCEPT_INTEL_RESERVED_6             23         // No
#define EXCEPT_INTEL_RESERVED_7             24         // No
#define EXCEPT_INTEL_RESERVED_8             25         // No
#define EXCEPT_INTEL_RESERVED_9             26         // No
#define EXCEPT_INTEL_RESERVED_10            27         // No
#define EXCEPT_INTEL_RESERVED_11            28         // No
#define EXCEPT_INTEL_RESERVED_12            29         // No
#define EXCEPT_INTEL_RESERVED_13            30         // No
#define EXCEPT_INTEL_RESERVED_14            31         // No


/*===========================================================================*
 * CPU privilege levels used by Simplix.                                     *
 * See Intel Developer's Manual Volume 3 - section 4.5                       *
 *===========================================================================*/

#define KERN_PRIVILEGE_LEVEL 0
#define USER_PRIVILEGE_LEVEL 3


/*===========================================================================*
 * Types of x86 segments used by Simplix.                                    *
 * See Intel Developer's Manual Volume 3 - table 3-1                         *
 *===========================================================================*/

#define GDT_CS_TYPE  0x9a /* present, system, DPL-0, execute/read     */
#define GDT_DS_TYPE  0x92 /* present, system, DPL-0, read/write       */
#define GDT_TSS_TYPE 0x89 /* present, system, DPL-0, 32-bit TSS       */
#define GDT_LDT_TYPE 0x82 /* present, system, DPL-0, LDT              */

#define LDT_CS_TYPE  0xfa /* present, non-system, DPL-3, execute/read */
#define LDT_DS_TYPE  0xf2 /* present, non-system, DPL-3, read/write   */


/*===========================================================================*
 * Global Descriptor Table (GDT)                                             *
 *===========================================================================*/

#define GDT_CS_INDEX  1 /* code segment           */
#define GDT_DS_INDEX  2 /* data segment           */
#define GDT_TSS_INDEX 3 /* task-state segment     */
#define GDT_LDT_INDEX 4 /* local descriptor table */

#define GDT_CS  SEG_REG_VAL(KERN_PRIVILEGE_LEVEL, 0, GDT_CS_INDEX)
#define GDT_DS  SEG_REG_VAL(KERN_PRIVILEGE_LEVEL, 0, GDT_DS_INDEX)
#define GDT_TSS SEG_REG_VAL(KERN_PRIVILEGE_LEVEL, 0, GDT_TSS_INDEX)
#define GDT_LDT SEG_REG_VAL(KERN_PRIVILEGE_LEVEL, 0, GDT_LDT_INDEX)


/*===========================================================================*
 * Local Descriptor Table (LDT)                                              *
 *===========================================================================*/

#define NR_LDT_ENTRIES 2

#define LDT_CS_INDEX 0 /* code segment */
#define LDT_DS_INDEX 1 /* data segment */

#define LDT_CS SEG_REG_VAL(USER_PRIVILEGE_LEVEL, 1, LDT_CS_INDEX)
#define LDT_DS SEG_REG_VAL(USER_PRIVILEGE_LEVEL, 1, LDT_DS_INDEX)


/*===========================================================================*
 * Constants related to physical memory management.                          *
 *===========================================================================*/

/* Number of bits necessary to encode the size of a physical memory page.
   This single macro defines the granularity of the physical memory manager. */
#define PAGE_BIT_SHIFT 12

/* Useful macros when dealing with physical memory pages. */
#define PAGE_SIZE ( 1 << PAGE_BIT_SHIFT)
#define PAGE_MASK (~0 << PAGE_BIT_SHIFT)
#define PAGE_ALIGN_INF(addr) ((unsigned) (addr) & PAGE_MASK)
#define PAGE_ALIGN_SUP(addr) (((unsigned) (addr) + (PAGE_SIZE - 1)) & PAGE_MASK)


/*===========================================================================*
 * Timer and time management.                                                *
 *===========================================================================*/

/* Clock frequency, in Hertz. */
#define HZ 1000

/* Scheduler frequency, expressed in number of ticks. */
#define SCHED_TICKS 10


/*===========================================================================*
 * Constants related to tasks and task management.                           *
 *===========================================================================*/

/* The pid of the idle task (a.k.a. the bootstrap thread) */
#define IDLE_TASK_PID 0

/* The pid of the init task. */
#define INIT_TASK_PID 1

/* Maximum pid value. */
#define MAX_PID 65535

/* Size of the kernel-space and user-space stacks, expressed both in number
   of physical memory pages and in bytes. */

#define KSTACK_PAGES 1
#define KSTACK_SIZE (KSTACK_PAGES << PAGE_BIT_SHIFT)

#define USTACK_PAGES 1
#define USTACK_SIZE (USTACK_PAGES << PAGE_BIT_SHIFT)

/* Task states. */
#define TASK_RUNNABLE        0
#define TASK_INTERRUPTIBLE   1
#define TASK_UNINTERRUPTIBLE 2
#define TASK_DEAD            3

/* Initial time slice expressed in number of timer ticks. */
#define INITIAL_TIMESLICE (10 * SCHED_TICKS)

/* Once the time slice of all runnable tasks (not including the idle task)
   has been exhausted, the time slice of all tasks is incremented by the
   following value. */
#define TIMESLICE_INCREMENT (3 * SCHED_TICKS)

/* Maximum time slice a task may accrue if it spends all of its time waiting.
   This is to avoid having an IO-bound task accrue a huge time slice before
   suddenly becoming CPU bound. */
#define MAX_TIMESLICE (15 * SCHED_TICKS)


/*===========================================================================*
 * System calls.                                                             *
 *===========================================================================*/

/* Exception number for a system call. */
#define SYSCALL_INT_NUM 0x80

/* Number of system calls. */
#define NR_SYSCALLS 9

/* List of system calls (value of EAX register) */
#define SYSCALL_EXIT        0
#define SYSCALL_FORK        1
#define SYSCALL_WAITPID     2
#define SYSCALL_GETPID      3
#define SYSCALL_GETPPID     4
#define SYSCALL_TIME        5
#define SYSCALL_STIME       6
#define SYSCALL_SLEEP       7
#define SYSCALL_BRK         8


/*===========================================================================*
 * Devices major number.                                                     *
 *===========================================================================*/

#define NR_BLKDEV_MAJOR_TYPES   2

#define BLKDEV_RAM_DISK_MAJOR   0
#define BLKDEV_IDE_DISK_MAJOR   1


/*===========================================================================*
 * Constants used by the text-mode video driver.                             *
 *===========================================================================*/

/* Screen geometry. */
#define SCREEN_ROWS 25
#define SCREEN_COLS 80

/* Text attributes. */
#define GFX_STATIC             0
#define GFX_BLINKING           1

/* Foreground and background colors. */
#define GFX_BLACK              0 /* 0000 */
#define GFX_BLUE               1 /* 0001 */
#define GFX_GREEN              2 /* 0010 */
#define GFX_CYAN               3 /* 0011 */
#define GFX_RED                4 /* 0100 */
#define GFX_MAGENTA            5 /* 0101 */
#define GFX_BROWN              6 /* 0110 */
#define GFX_LIGHT_GRAY         7 /* 0111 */

/* Foreground-only colors. */
#define GFX_DARK_GRAY          8 /* 1000 */
#define GFX_LIGHT_BLUE         9 /* 1001 */
#define GFX_LIGHT_GREEN       10 /* 1010 */
#define GFX_LIGHT_CYAN        11 /* 1011 */
#define GFX_LIGHT_RED         12 /* 1100 */
#define GFX_LIGHT_MAGENTA     13 /* 1101 */
#define GFX_YELLOW            14 /* 1110 */
#define GFX_WHITE             15 /* 1111 */

/* Light gray text on a black background. */
#define DEFAULT_TEXT_ATTR  GFX_ATTR(GFX_LIGHT_GRAY, GFX_BLACK, GFX_STATIC)


/*===========================================================================*
 * Constants used by the keyboard driver.                                    *
 *===========================================================================*/

/* Shift key offset. */
#define SHIFT 0x0100

#define ESCAPE            27
#define DELETE           127

/* Function keys. */

#define F1              0x81
#define F2              0x82
#define F3              0x83
#define F4              0x84
#define F5              0x85
#define F6              0x86
#define F7              0x87
#define F8              0x88
#define F9              0x89
#define F10             0x8a
#define F11             0x8b
#define F12             0x8c

/* Numeric keypad. */

#define HOME            0x8d
#define END             0x8e
#define UP              0x8f
#define DOWN            0x90
#define LEFT            0x91
#define RIGHT           0x92
#define PGUP            0x93
#define PGDN            0x94
#define CENTER          0x95
#define INSERT          0x96

/* Extension keys. */

#define LSHIFT          0x97
#define RSHIFT          0x98
#define CTRL            0x99
#define ALT             0x9a

/* Lock keys. */

#define CAPS_LOCK       0x9b
#define NUM_LOCK        0x9c
#define SCR_LOCK        0x9d


#endif /* _SIMPLIX_CONST_H_ */
