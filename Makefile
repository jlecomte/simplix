AS = as
CC = gcc
LD = ld
RM = rm -f
DD = dd
OBJCOPY = objcopy
STRIP = strip
SUDO = sudo
PWD = $(shell pwd)/include/
FLOPPY = /dev/sdb
CFLAGS = -Wall -m32 -I$(PWD) -nostdlib -nostdinc -ffreestanding -fno-stack-protector
LDFLAGS = -T simplix.lds --warn-common

# libgcc.a is used for 64 bit division/modulo operations on 32 bit arch.
# Note: If you're building this on a 64 bit system, you'll need the multilib
# version of gcc. On Ubuntu 8.04 (Hardy), install the gcc-multilib package.
LIBGCC = $(shell $(CC) -m32 -print-libgcc-file-name)

OBJS = boot/bootsect_asm.o          \
       drivers/gfx.o                \
       drivers/ide.o                \
       drivers/kbd.o                \
       drivers/ramdisk.o            \
       kernel/blkdev.o              \
       kernel/exception.o           \
       kernel/gdt.o                 \
       kernel/idt.o                 \
       kernel/irq.o                 \
       kernel/isr_asm.o             \
       kernel/kmem.o                \
       kernel/ksync.o               \
       kernel/main.o                \
       kernel/physmem.o             \
       kernel/sched.o               \
       kernel/sys.o                 \
       kernel/syscall_asm.o         \
       kernel/task_switch_asm.o     \
       kernel/task.o                \
       kernel/timer.o               \
       lib/stdlib.o                 \
       lib/string.o

all: floppy.img

debug: CFLAGS += -ggdb
debug: floppy.img

clean:
	$(RM) lib/*.o
	$(RM) kernel/*.o
	$(RM) boot/*.o boot/*.s
	$(RM) drivers/*.o drivers/*.s
	$(RM) bootsect.bin kernel.bin simplix.elf floppy.img out.bochs

floppy.img: bootsect.bin kernel.bin
	$(DD) if=/dev/zero of=floppy.img bs=512 count=2880
	$(DD) if=bootsect.bin of=floppy.img conv=notrunc
	$(DD) if=kernel.bin of=floppy.img conv=notrunc seek=1

floppy: bootsect.bin kernel.bin
	$(SUDO) $(DD) if=bootsect.bin of=$(FLOPPY) conv=notrunc
	$(SUDO) $(DD) if=kernel.bin of=$(FLOPPY) conv=notrunc seek=1

%_asm.o: %.S
	$(CC) $(CFLAGS) -DASM_SOURCE=1 -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

simplix.elf: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBGCC)

bootsect.bin: simplix.elf
	$(OBJCOPY) -v -O binary -j .bootsect $< $@

kernel.bin: simplix.elf 
	$(OBJCOPY) -v -O binary -R .bootsect $< $@
