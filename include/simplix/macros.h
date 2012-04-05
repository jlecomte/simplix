/*===========================================================================
 *
 * macros.h
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
 * Miscellaneous macros and inline functions.
 *
 *===========================================================================*/

#ifndef _SIMPLIX_MACROS_H_
#define _SIMPLIX_MACROS_H_

#include <simplix/consts.h>

/* Miscellaneous inline assembly macros. */
#define idle()  asm("1: jmp 1b")
#define hlt()   asm("hlt")
#define cli()   asm("cli")
#define sti()   asm("sti")

/*
 * The macro disable_hwint disables all hardware interrupts. The macro
 * enable_hwint enables them. However, consider the following snippet:
 *
 * -> We need to make sure that hardware interrupts are disabled before
 *    entering a critical section, so we call disable_hwint:
 * disable_hwint
 *
 *     ... do something that requires hardware interrupts to be disabled ...
 *
 *     function call
 *         ...
 *         -> We need to enter a critical section. Since this function can be
 *            called from any context, the only way to make sure that hardware
 *            interrupts are indeed disabled is to call disable_hwint:
 *         disable_hwint
 *
 *         ... do something that requires hardware interrupts to be disabled ...
 *
 *         -> End the critical section by re-enabling hardware interrupts:
 *         restore_hwint
 *         ...
 *     return from the function
 *
 *     -> At this point, we expect hardware interrupts to still be disabled.
 *        However, enable_hwint was used in the function that we just called.
 *        This demonstrates the fact that calls to disable_hwint / enable_hwint
 *        can be imbricated and enable_hwint may or may not actually re-enable
 *        hardware interrupts depending on where we are.
 *
 *     ... do something that requires hardware interrupts to be disabled ...
 *
 * -> End the critical section by re-enabling hardware interrupts:
 * restore_hwint
 *
 * How do we do that? Bit 9 of the EFLAGS register is set if hardware
 * interrupts are enabled. We simply need to "remember" its value when
 * calling disable_hwint. Depending on that value, we will or will not
 * re-enable hardware interrupts.
 */

#define disable_hwint(eflags) asm("pushf; pop %0; cli;" : "=g" ((eflags)))
#define restore_hwint(eflags) asm("push %0; popf;" :: "g" ((eflags)))

/* Usually, when handling a timeout, you would write something like:

       unsigned long timeout = ticks + xxx;
       // Do some work here...
       if (timeout > ticks) {
           // we haven't timed out yet...
       } else {
           // we timed out!
       }

   In the Simplix kernel, we keep track of time by incrementing a 32-bit counter
   at each clock tick i.e. every millisecond. This means that the counter wraps
   around (i.e. overflows) every 50 days. If you set a timeout right before the
   counter wraps around, you may have to wait a very long time for your timeout
   to expire using the code above. Assuming that you don't use timeouts greater
   than 50 days (you shouldn't do that anyway...), it is possible to handle the
   wraparound by using the following macros. I stole these from the Linux kernel
   (see linux/jiffies.h) These macros rely on signed integer arithmetics.
*/

#define time_before(a, b)       (((long) a - (long) b) < 0)
#define time_before_eq(a, b)    (((long) a - (long) b) <= 0)
#define time_after(a, b)        time_before(b, a)
#define time_after_eq(a, b)     time_before_eq(b, a)

/* Video text attributes. */
#define GFX_ATTR(textcolor, bgcolor, blinking) \
    ((blinking << 7) | (bgcolor << 4) | textcolor)

#endif /* _SIMPLIX_MACROS_H_ */
