/*===========================================================================
 *
 * timer.c
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
 * This file contains code responsible for keeping track of time, as well as
 * miscellaneous time-related functions.
 *
 *===========================================================================*/

#include <simplix/assert.h>
#include <simplix/consts.h>
#include <simplix/globals.h>
#include <simplix/io.h>
#include <simplix/list.h>
#include <simplix/macros.h>
#include <simplix/proto.h>
#include <simplix/types.h>

/* PIT (Programmable Interrupt Timer) constants. */
#define PIT_CHANNEL0  0x40
#define PIT_COMMAND   0x43
#define PIT_FREQUENCY 1193180

/* RTC (Real Time Clock) constants. */
#define RTC_COMMAND 0x70
#define RTC_DATA    0x71
#define RTC_SECOND     0
#define RTC_MINUTE     2
#define RTC_HOUR       4
#define RTC_DATE       7
#define RTC_MONTH      8
#define RTC_YEAR       9
#define RTC_STATUS    10

/* Useful time-related macros and inline functions. */
#define SECONDS_PER_MINUTE                60
#define SECONDS_PER_HOUR                3600
#define SECONDS_PER_DAY                86400
#define SECONDS_PER_YEAR            31536000
#define SECONDS_PER_LEAP_YEAR       31622400

static inline bool_t is_leap(int year)
{
    return year >= 1582 ?
        ((year % 4 == 0) &&
         ((year % 100 != 0) ||
          (year % 400 == 0))) :
        (year % 4 == 0);
}

static inline unsigned int nr_days_per_year(int year)
{
    return is_leap(year) ? 366 : 365;
}

static inline unsigned int nr_days_per_month(unsigned int month, int year)
{
    ASSERT(month < 12);

    unsigned int days_per_month[] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };

    return is_leap(year) && month == 1 ? 29 : days_per_month[month];
}

/* Converts BCD (Binary Coded Decimal) to decimal. */
static inline unsigned int bcd_to_dec(int n)
{
    unsigned int res = 0, mul = 1;
    while (n) {
        res += (n & 0xf) * mul;
        mul *= 10;
        n >>= 4;
    }
    return res;
}

/* Forward declarations. */
static void handle_timer_interrupt(uint32_t esp);

/* Number of clock ticks since the system started. This number wraps around
   approximately every 50 days. */
unsigned long ticks = 0;

/* Unix time i.e. number of seconds since the Epoch, which is defined as
   0:00:00 UTC on the morning of 1 January 1970. This is a signed 64-bit
   value, so we can represent pretty much any date we need in the past
   or in the future. */
time_t realtime;

void init_timer(void)
{
    /* Calculate our divisor. */
    uint16_t divisor = PIT_FREQUENCY / HZ;

    /* Channel 0, LSB/MSB, mode 2, binary. */
    outb(PIT_COMMAND, 0x34); udelay(1);

    /* Set low byte of divisor. */
    outb(PIT_CHANNEL0, divisor & 0xff); udelay(1);

    /* Set high byte of divisor. */
    outb(PIT_CHANNEL0, divisor >> 8); udelay(1);

    /* Set the clock IRQ handler. */
    irq_set_handler(IRQ_TIMER, handle_timer_interrupt);

    /* Enable the corresponding IRQ line. */
    enable_irq_line(IRQ_TIMER);
}

/*
 * Get the time from the PC's real time clock and computes the Unix time.
 * Note: We assume that the PC's real time clock is set to UTC (see .bochsrc)
 * See http://www.cl.cam.ac.uk/~mgk25/mswish/ut-rtc.html
 */
void init_wall_clock(void)
{
    byte_t status;
    unsigned int second, minute, hour, date, month, year, m, y;

    do {
        /* Don't read from CMOS if an update is in progress. */
        outb(RTC_COMMAND, RTC_STATUS);
        status = inb(RTC_DATA);
    } while (status & 0x80);

    outb(RTC_COMMAND, RTC_SECOND);
    second = bcd_to_dec(inb(RTC_DATA));

    outb(RTC_COMMAND, RTC_MINUTE);
    minute = bcd_to_dec(inb(RTC_DATA));

    outb(RTC_COMMAND, RTC_HOUR);
    hour = bcd_to_dec(inb(RTC_DATA));

    outb(RTC_COMMAND, RTC_DATE);
    date = bcd_to_dec(inb(RTC_DATA));

    outb(RTC_COMMAND, RTC_MONTH);
    month = bcd_to_dec(inb(RTC_DATA));

    /* Note: We assume we are in the 21st century now. */
    outb(RTC_COMMAND, RTC_YEAR);
    year = 2000 + bcd_to_dec(inb(RTC_DATA));

    /* Compute Unix time. */

    realtime = 0;

    for (y = 1970; y < year; y++)
        realtime += nr_days_per_year(y) * SECONDS_PER_DAY;

    for (m = 0; m < month-1; m++)
        realtime += nr_days_per_month(m, year) * SECONDS_PER_DAY;

    realtime += (date - 1) * SECONDS_PER_DAY;
    realtime += hour * SECONDS_PER_HOUR;
    realtime += minute * SECONDS_PER_MINUTE;
    realtime += second;
}

/*
 * The system clock IRQ handler.
 */
static void handle_timer_interrupt(uint32_t esp)
{
    int i;
    struct task_struct *t;
    static int realtime_ticks = HZ;
    static int sched_ticks = SCHED_TICKS;

    /* Increment the global tick count. */
    ticks++;

    /* Update the current process CPU time. */
    current->cputime++;

    /* Update the current task's time slice. */
    if (current->pid != IDLE_TASK_PID && current->timeslice)
        current->timeslice--;

    /* Do we need to update the real time? */
    if (--realtime_ticks == 0) {
        realtime_ticks = HZ;
        realtime++;
    }

    /* Check for any expired timeouts. */
    list_for_each(task_list_head, t, i)
        if (t->timeout) {
            ASSERT(t->state == TASK_UNINTERRUPTIBLE);
            t->timeout--;
            if (!t->timeout)
                t->state = TASK_RUNNABLE;
        }

    if (--sched_ticks == 0) {
        sched_ticks = SCHED_TICKS;
        schedule();
    }
}
