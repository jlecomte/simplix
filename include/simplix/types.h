/*===========================================================================
 *
 * types.h
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

#ifndef _SIMPLIX_TYPES_H_
#define _SIMPLIX_TYPES_H_

/*===========================================================================
 *
 * Fixed width types.
 *
 * When doing any kind of low level system development, one can't use
 * the standard C data types because C data types are not the same size
 * on all architectures. The following types are defined (differently)
 * for all architectures to provide a common set of data types.
 *
 * From the "Linux Device Drivers" book (chapter 11) available at
 * http://lwn.net/Kernel/LDD3/
 *
 * arch    Size: char short int long ptr long-long u8  u16 u32 u64
 * i386            1    2    4    4   4     8       1   2   4   8
 * alpha           1    2    4    8   8     8       1   2   4   8
 * armv4l          1    2    4    4   4     8       1   2   4   8
 * ia64            1    2    4    8   8     8       1   2   4   8
 * m68k            1    2    4    4   4     8       1   2   4   8
 * mips            1    2    4    4   4     8       1   2   4   8
 * ppc             1    2    4    4   4     8       1   2   4   8
 * sparc           1    2    4    4   4     8       1   2   4   8
 * sparc64         1    2    4    4   4     8       1   2   4   8
 * x86_64          1    2    4    8   8     8       1   2   4   8
 *
 *===========================================================================*/

typedef char sint8_t;
typedef unsigned char uint8_t;

typedef short sint16_t;
typedef unsigned short uint16_t;

typedef int sint32_t;
typedef unsigned int uint32_t;

typedef long long sint64_t;
typedef unsigned long long uint64_t;


/*===========================================================================*
 * Basic types used by the kernel.                                           *
 *===========================================================================*/

typedef uint32_t addr_t;
typedef uint32_t size_t;
typedef uint32_t offset_t;
typedef uint64_t loffset_t;
typedef uint32_t time_t;
typedef sint32_t pid_t;
typedef uint8_t  ret_t;
typedef uint8_t  byte_t;

typedef enum { FALSE=0, TRUE } bool_t;

#define NULL ((void*)0)


/*===========================================================================*
 * Complex types used by the kernel.                                         *
 *===========================================================================*/

/* IRQ handler. */
typedef void (* irq_handler_t)(uint32_t esp);

/* Exception handler. */
typedef void (* exception_handler_t)(uint32_t esp);

/* Kernel thread entry point. */
typedef void (* task_entry_point_t)(void);


#endif /* _SIMPLIX_TYPES_H_ */
