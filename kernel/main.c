/*===========================================================================
 *
 * main.c
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
 * This is the SIMPLIX kernel entry point.
 *
 *===========================================================================*/

#include <string.h>
#include <stdlib.h>
#include <syscalls.h>
#include <vararg.h>

#include <simplix/consts.h>
#include <simplix/globals.h>
#include <simplix/list.h>
#include <simplix/io.h>
#include <simplix/macros.h>
#include <simplix/proto.h>
#include <simplix/types.h>

/* Number of rows displaying system information. Tasks can start displaying
   information below this row. This is used only for the test tasks. */
static int gfx_base_row;

/* Forward declarations. */
static void ide_driver_test_task(void);
static void clock_task(void);
static void prime_numbers_task(void);
static void system_stat_task(void);
static void init_task(void);
static void process_demo_task(void);
static void compute_pi_task();

/*
 * The Simplix kernel entry point.
 */
void simplix_main(uint32_t memsize)
{
    pid_t pid;
    char msg[256];

    /* Initialize the PICs. */
    init_pic();

    /* Initialize the IDT. */
    init_idt();

    /* Initialize our own GDT. */
    init_gdt();

    /* Initialize the text-mode video driver. */
    init_gfx();

    /* Print a welcome message. */
    gfx_putstring("Welcome to SIMPLIX!\n");
    snprintf(msg, 256, "System has %u Bytes of physical memory\n", memsize);
    gfx_putstring(msg);

    /* Initialize physical memory allocator. */
    init_physmem(memsize);

    /* Initialize hard drives. */
    init_ide_devices();

    /* Initialize RAM disk driver. */
    init_ramdisk_driver();

    gfx_base_row = gfx_get_cursor_offset() / SCREEN_COLS;

    /* Initialize keyboard driver. */
    init_kbd();

    /* Handle exceptions. */
    init_exceptions();

    /* Initialize the time-tracking subsystem. */
    init_timer();
    init_wall_clock();

    /* Start a few kernel threads, while we can. */
    kernel_thread(ide_driver_test_task);
    kernel_thread(clock_task);
    kernel_thread(prime_numbers_task);
    kernel_thread(system_stat_task);

    /* Initialize the multitasking subsystem. */
    init_multitasking();

    /* The base system is now initialized. We have morphed into the idle task,
       and are now in user space. Fork and execute the init task to complete
       system initialization. */

    pid = fork();
    if (pid == 0) {
        /* This is the init task. */
        init_task();
    } else {
        /* Note: We assume the above fork has succeeded!
           This is the idle task. Just spin until the end of time. */
        idle();
    }
}

int printk(const char *format, ...)
{
    #define BOCHS_IOPORT 0xe9

    int len;
    va_list ap;
    char buf[256];
    char *s = buf;

    va_start(ap, format);
    len = vsnprintf(buf, sizeof(buf), format, ap);

    while (*s != '\0')
        outb(BOCHS_IOPORT, *s++);

    return len;
}

void panic(const char *format, ...)
{
    va_list ap;

    cli();
    va_start(ap, format);
    printk("KERNEL PANIC: ");
    printk(format, ap);
    hlt();
}

/*
 * This kernel thread tests the IDE driver. Along with the Simplix distribution
 * comes a disk image file (disk.img) and a Bochs configuration file (.bochsrc)
 * The disk image should be seen by Bochs as a disk drive connected to the first
 * IDE controller in master position, with some sparse data in its first block.
 * This task reads the first block of the master IDE device on the first IDE
 * controller, and outputs the last two bytes. These should be 0xaa and 0x55
 * which is the BIOS boot record signature on a PC. You should get the same
 * result on a real PC.
 */
static void ide_driver_test_task(void)
{
    byte_t buffer[512];
    char msg[256];

    if (blkdev_read(BLKDEV_IDE_DISK_MAJOR, 0, 0, 512, buffer) == S_OK) {
        snprintf(msg, sizeof(msg),
            "Reading 1st sector of IDE device [0-0]: OK (signature is 0x%.02X%.02X)",
            buffer[510], buffer[511]);
        videomem_putstring(msg, gfx_base_row, 0, DEFAULT_TEXT_ATTR);
    } else {
        videomem_putstring("Reading 1st sector of IDE device [0-0]: Error",
            gfx_base_row, 0, DEFAULT_TEXT_ATTR);
    }

    do_exit(0);
}

/*
 * A kernel thread that displays a clock and updates it every second. Note that
 * making the thread sleep for a second and then update the display is not the
 * right way of doing this kind of thing. Indeed, the thread is not guaranteed
 * to sleep for exactly one second (actually, it'll sleep a little more than a
 * second) so a small delay will accumulate over time. The time displayed will
 * always be accurate at the time it is adjusted, but at some point in time,
 * the display will skip a second. No big deal, this is just an example...
 */
static void clock_task(void)
{
    char buf[256];

    for (;;) {
        snprintf(buf, sizeof(buf), "Current Unix time is %u", realtime);
        /* Display the clock on the 4th line. */
        videomem_putstring(buf, gfx_base_row + 1, 0, DEFAULT_TEXT_ATTR);
        do_sleep(1000);
    }
}

/*
 * A kernel thread that finds prime numbers by using the most trivial algorithm
 * possible. Efficiency is not a concern here. We just need something that is
 * CPU intensive to test the scheduling algorithm.
 */
static void prime_numbers_task(void)
{
    char buf[256];
    int found;
    unsigned long num = 1, div;

    for (;;) {
        num++;
        found = 1;
        for (div = 2; div < num; div++)
            if (num % div == 0)
                found = 0;
        if (found) {
            snprintf(buf, sizeof(buf), "Largest computed prime number: %u", num);
            /* Display the clock on the 5th line. */
            videomem_putstring(buf, gfx_base_row + 2, 0, DEFAULT_TEXT_ATTR);
        }
    }
}

/*
 * This kernel thread shows live system information on screen.
 */
static void system_stat_task(void)
{
    int i, row, col;
    char buf[256];
    struct task_struct *t;
    unsigned long cpu_usage;
    unsigned long last_tick;
    unsigned long eflags;

    /* Print the header. */
    #define HDR_TEXT_ATTR GFX_ATTR(GFX_BLACK, GFX_WHITE, GFX_STATIC)
    for (col = 0; col < SCREEN_COLS; col++)
        videomem_putchar(' ', gfx_base_row + 3, col, HDR_TEXT_ATTR);
    videomem_putstring("  PID   %CPU", gfx_base_row + 3, 0, HDR_TEXT_ATTR);

    for (;;) {

        last_tick = ticks;
        do_sleep(1000);

        /* Clear the screen below the header. */
        for (row = gfx_base_row + 4; row < SCREEN_ROWS; row++)
            for (col = 0; col < SCREEN_COLS; col++)
                videomem_putchar(0, row, col, DEFAULT_TEXT_ATTR);

        /* Entering critical section... */
        disable_hwint(eflags);

        row = gfx_base_row + 4;
        list_for_each(task_list_head, t, i) {
            if (t->state == TASK_DEAD)
                continue;
            cpu_usage = (t->cputime * 100) / (ticks - last_tick);
            t->cputime = 0;
            snprintf(buf, sizeof(buf), "%.5u    %.3u", t->pid, cpu_usage);
            videomem_putstring(buf, row++, 0, DEFAULT_TEXT_ATTR);
        }

        /* Leaving critical section... */
        restore_hwint(eflags);
    }
}

/*
 * The init task. Right now, we don't have any user mode IO available, so we
 * just try to raise some exceptions, and test the fork and exit system calls.
 * Take a look at bochs' standard output.
 */
static void init_task(void)
{
    pid_t pid;
    int status;

    /* Start a user task that computes the number PI. */
    pid = fork();
    if (pid == 0) {
        compute_pi_task();
        exit(0);
    }

    /* Start a demo task. */
    pid = fork();
    if (pid == 0) {
        process_demo_task();
        exit(0);
    }

    /* Clean up child tasks that have terminated. */
    for (;;) {
        waitpid(-1, &status);
        sleep(1000);
    }
}

/*
 * Demos memory protection and the fork, sleep and exit system calls.
 */
static void process_demo_task(void)
{
    pid_t pid;
    char *c = (char *) 0xffffff;
    int n = 0, val;

    /* Test: Generate a General Protection Exception. */
    pid = fork();
    if (pid == 0) {
        *c = 'a';
        exit(0);
    }

    /* Test: Generate a Divide Error Exception. */
    pid = fork();
    if (pid == 0) {
        val = 1 / n;
        exit(0);
    }

    /* Test: fork/exit user tasks that spend their time sleeping. */
    for (;;) {
        pid = fork();
        sleep(1000);
        if (pid != 0) {
            sleep(1000);
            exit(0);
        }
    }
}

/*
 * A user task that computes decimals of the number PI.
 *
 * Pascal Sebah : September 1999
 *
 * Subject:
 *
 *    A very easy program to compute Pi with many digits.
 *    No optimisations, no tricks, just a basic program to learn how
 *    to compute in multiprecision.
 *
 * Formulae:
 *
 *    Pi/4 =    arctan(1/2)+arctan(1/3)                     (Hutton 1)
 *    Pi/4 =  2*arctan(1/3)+arctan(1/7)                     (Hutton 2)
 *    Pi/4 =  4*arctan(1/5)-arctan(1/239)                   (Machin)
 *    Pi/4 = 12*arctan(1/18)+8*arctan(1/57)-5*arctan(1/239) (Gauss)
 *
 *      with arctan(x) =  x - x^3/3 + x^5/5 - ...
 *
 *    The Lehmer's measure is the sum of the inverse of the decimal
 *    logarithm of the pk in the arctan(1/pk). The more the measure
 *    is small, the more the formula is efficient.
 *    For example, with Machin's formula:
 *
 *      E = 1/log10(5)+1/log10(239) = 1.852
 *
 * Data:
 *
 *    A big real (or multiprecision real) is defined in base B as:
 *      X = x(0) + x(1)/B^1 + ... + x(n-1)/B^(n-1)
 *      where 0<=x(i)<B
 *
 * Results: (PentiumII, 450Mhz)
 *
 *   Formula      :    Hutton 1  Hutton 2   Machin   Gauss
 *   Lehmer's measure:   5.418     3.280      1.852    1.786
 *
 *  1000   decimals:     0.2s      0.1s       0.06s    0.06s
 *  10000  decimals:    19.0s     11.4s       6.7s     6.4s
 *  100000 decimals:  1891.0s   1144.0s     785.0s   622.0s
 *
 * With a little work it's possible to reduce those computation
 * times by a factor 3 and more:
 *
 *     => Work with double instead of long and the base B can
 *        be choosen as 10^8
 *     => During the iterations the numbers you add are smaller
 *        and smaller, take this in account in the +, *, /
 *     => In the division of y=x/d, you may precompute 1/d and
 *        avoid multiplications in the loop (only with doubles)
 *     => MaxDiv may be increased to more than 3000 with doubles
 *     => ...
 */

#define abs(n) (n < 0 ? -n : n)

static long B = 10000;    /* Working base */
static long LB = 4;       /* Log10(base)  */
static long MaxDiv = 450; /* about sqrt(2^31/B) */

/*
 * Set the big real x to the small integer Integer
 */
static void SetToInteger(long n, long *x, long Integer)
{
    long i;
    for (i = 1; i < n; i++)
        x[i] = 0;
    x[0] = Integer;
}

/*
 * Is the big real x equal to zero ?
 */
static long IsZero(long n, long *x)
{
    long i;
    for (i = 0; i < n; i++)
        if (x[i])
            return 0;
    return 1;
}

/*
 * Addition of big reals : x += y
 * Like school addition with carry management
 */
static void Add(long n, long *x, long *y)
{
    long carry = 0, i;
    for (i = n-1; i >= 0; i--) {
        x[i] += y[i]+carry;
        if (x[i]<B) {
            carry = 0;
        } else {
            carry = 1;
            x[i] -= B;
        }
    }
}

/*
 * Substraction of big reals : x -= y
 * Like school substraction with carry management
 * x must be greater than y
 */
static void Sub(long n, long *x, long *y)
{
    long i;
    for (i = n-1; i >= 0; i--) {
        x[i] -= y[i];
        if (x[i]<0) {
            if (i) {
                x[i] += B;
                x[i-1]--;
            }
        }
    }
}

/*
 * Multiplication of the big real x by the integer q : x = x*q.
 * Like school multiplication with carry management
 */
static void Mul(long n, long *x, long q)
{
    long carry = 0, xi, i;
    for (i = n-1; i >= 0; i--) {
        xi = x[i]*q;
        xi += carry;
        if (xi>=B) {
            carry = xi/B;
            xi -= (carry*B);
        } else {
            carry = 0;
        }
        x[i] = xi;
    }
}

/*
 * Division of the big real x by the integer d: y = x/d
 * Like school division with carry management
 * d is limited to MaxDiv*MaxDiv.
 */
static void Div(long n, long *x, long d, long *y)
{
    long carry = 0, xi, q, i;
    for (i = 0; i < n; i++) {
        xi = x[i]+carry*B;
        q = xi/d;
        carry = xi-q*d;
        y[i] = q;
    }
}

/*
 * Find the arc cotangent of the integer p = arctan (1/p)
 * Result in the big real x (size n)
 * buf1 and buf2 are two buffers of size n
 */
static void arccot(long p, long n, long *x, long *buf1, long *buf2)
{
    long p2 = p*p, k = 3, sign = 0;
    long *uk = buf1, *vk = buf2;
    SetToInteger(n, x, 0);
    SetToInteger(n, uk, 1); /* uk = 1/p */
    Div(n, uk, p, uk);
    Add(n, x, uk); /* x  = uk */

    while (!IsZero(n, uk)) {
        if (p < MaxDiv) {
            Div(n, uk, p2, uk); /* One step for small p */
        } else {
            Div(n, uk, p, uk);  /* Two steps for large p (see division) */
            Div(n, uk, p, uk);
        }
        /* uk = u(k-1)/(p^2) */
        Div(n, uk, k, vk);  /* vk = uk/k  */
        if (sign) {
            Add (n, x, vk); /* x = x+vk   */
        } else {
            Sub (n, x, vk); /* x = x-vk   */
        }
        k += 2;
        sign = 1-sign;
    }
}

static void compute_pi_task()
{
    long i, p[10], m[10], NbArctan, NbDigits = 1000, size = 1 + NbDigits / LB;

    long *pi      = (long *) malloc(size * sizeof(long));
    long *arctan  = (long *) malloc(size * sizeof(long));
    long *buffer1 = (long *) malloc(size * sizeof(long));
    long *buffer2 = (long *) malloc(size * sizeof(long));

    if (!pi || !arctan || !buffer1 || !buffer2)
        exit(1);

    /*
     * Formula used:
     *
     * pi/4 = 12*arctan(1/18)+8*arctan(1/57)-5*arctan(1/239) (Gauss)
     */

    NbArctan = 3;
    m[0] = 12; m[1] = 8;  m[2] = -5;
    p[0] = 18; p[1] = 57; p[2] = 239;
    SetToInteger(size, pi, 0);

    /*
     * Computation of pi/4 = Sum(i) [m[i]*arctan(1/p[i])]
     */

    for (i = 0; i < NbArctan; i++) {
        arccot(p[i], size, arctan, buffer1, buffer2);
        Mul(size, arctan, abs(m[i]));
        if (m[i] > 0) {
            Add(size, pi, arctan);
        } else {
            Sub(size, pi, arctan);
        }
    }

    Mul(size, pi, 4);

    free(pi);
    free(arctan);
    free(buffer1);
    free(buffer2);
}
