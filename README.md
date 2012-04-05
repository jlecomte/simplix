# Simplix

## Introduction

Back in university, I took a few classes dealing with operating system design.
These classes were extremely theoretical and, in some ways, helped me
throughout my curriculum and my career, serving as a solid base I could then
build upon to gain new knowledge. However, after spending a few years working
with high level languages in sandboxed environments, one tends to forget how
things work at lower levels, and that sometimes leads to less than optimal
higher level code.

In 2007, faced with that situation, I decided it was time for me to brush up on
my core CS skills. However, I needed a tangible goal. And then I thought: why
not write an operating system? OK, not a full blown operating system of course,
but the seed of a very basic one (calling Simplix an operating system is a bit
of a stretch since it cannot be used for anything actually useful) One that
other people could look at and actually understand (Even MINIX, which was
designed to be easy to understand by students, is not that easy to grasp
without spending a lot of time hunched over the code)

## High level characteristics

* Target architecture: PC with a single Intel 386 or better CPU
* Monolithic, interruptible, non preemptible kernel
* Hardware interrupt handling using the Intel 8259 PIC
* Software interrupt handling
* Advanced management of physical memory:

    * High performance page allocator
    * High performance memory allocator (kmalloc/kfree)

* Support for kernel threads and user space processes
* Simple scheduling algorithm
* Support for virtual memory using segmentation
* Support for system calls: exit, fork, waitpid, getpid, getppid, time, stime, sleep and brk.
* Peripherals: keyboard, video screen
* Basic IDE device driver and RAM disk driver
* Basic user space library
