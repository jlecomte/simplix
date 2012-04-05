/*===========================================================================
 *
 * syscalls.h
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

#ifndef _SYSCALLS_H_
#define _SYSCALLS_H_

#include <simplix/consts.h>
#include <simplix/types.h>

static inline void exit(int status)
{
    asm("int %2"
        :
        : "a" (SYSCALL_EXIT),
          "b" (status),
          "i" (SYSCALL_INT_NUM));
}

static inline pid_t fork(void)
{
    pid_t pid;
    asm("int %2"
        : "=a" (pid)
        : "a" (SYSCALL_FORK),
          "i" (SYSCALL_INT_NUM));
    return pid;
}

static inline pid_t waitpid(pid_t pid, int *status)
{
    asm("int %4"
        : "=a" (pid)
        : "a" (SYSCALL_WAITPID),
          "b" (pid),
          "c" (status),
          "i" (SYSCALL_INT_NUM));
    return pid;
}

static inline pid_t getpid(void)
{
    pid_t pid;
    asm("int %2"
        : "=a" (pid)
        : "a" (SYSCALL_GETPID),
          "i" (SYSCALL_INT_NUM));
    return pid;
}

static inline pid_t getppid(void)
{
    pid_t ppid;
    asm("int %2"
        : "=a" (ppid)
        : "a" (SYSCALL_GETPPID),
          "i" (SYSCALL_INT_NUM));
    return ppid;
}

static inline time_t time(void)
{
    time_t t;
    asm("int %2"
        : "=a" (t)
        : "a" (SYSCALL_TIME),
          "i" (SYSCALL_INT_NUM));
    return t;
}

static inline int stime(time_t *t)
{
    int res;
    asm("int %3"
        : "=a" (res)
        : "a" (SYSCALL_STIME),
          "b" (t),
          "i" (SYSCALL_INT_NUM));
    return res;
}

static inline void sleep(unsigned int msec)
{
    asm("int %2"
        :
        : "a" (SYSCALL_SLEEP),
          "b" (msec),
          "i" (SYSCALL_INT_NUM));
}

static inline size_t brk(size_t data_segment_size)
{
    size_t res;
    asm("int %3"
        : "=a" (res)
        : "a" (SYSCALL_BRK),
          "b" (data_segment_size),
          "i" (SYSCALL_INT_NUM));
    return res;
}

#endif /* _SYSCALLS_H_ */
