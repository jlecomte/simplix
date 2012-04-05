/******************************************************************************
 * bootsect.S
 * Note: We assume that we have an Intel 386+ compatible CPU. Doing a good
 * CPU check is not trivial, especially under such strict size constraints.
 * Furthermore, it is not that useful nowadays.
 ******************************************************************************/

    #define BLOCK_SIZE            512
    #define LO_BOOT_SEG           0x7c0
    #define HI_BOOT_SEG           0x9f00
    #define BOOTLOADER_FINAL_ADDR 0x9f000
    /* Don't change this value without changing simplix.lds */
    #define KERNEL_ADDR           0x1000
    /* Maximum size allowed for the kernel, in number of 512 byte blocks */
    #define MAX_KERNEL_SIZE       ((BOOTLOADER_FINAL_ADDR - KERNEL_ADDR) / 512)
    #define DISK_READ_RETRIES     3

    #define PRINT(msg) mov $msg, %si; call print

    /* Keep in mind that the boot loader is linked to the kernel. However,
       we don't want any kernel code in our boot loader (we have only 512
       bytes!), so in the boot loader linker script, we must be able to
       specify exactly what to include in the final boot sector image. This
       is done by wrapping all the boot loader code within one section, and
       give that section a special name, so we can refer to it in the boot
       loader linker script. We could also have specified which file to
       include in the boot loader linker script... */

    .section .bootsect

    /* Tell the compiler that our code starts at offset 0. By doing so, our code
       can easily be relocated by changing the value of the segment register.
       This offset is also passed to the linker using the -Ttext option. */

    .org 0

    /* Declare our global symbols. */

    .globl start

    /* Tell the compiler to generate 16-bit code (we are in real mode) */

    .code16

start:

    jmp realstart

/******************************************************************************
 * Boot parameters block
 * This is filled by the patchboot program. It is located at the top of
 * the code so that it is easily located by that same program.
 ******************************************************************************/

/* This is the location of the boot block on disk using CHS addressing.
   Note that the default value works for a non partitioned disk, where
   the boot block is located in the first sector of the disk. Also,
   note that the cylinder value cannot exceed 256. This makes computation
   in loadkernel a whole lot easier. A rock solid boot loader would use
   LBA if available anyway... */
BOOT_BLOCK_CYLINDER_ADDR: .byte 0
BOOT_BLOCK_HEAD_ADDR:     .byte 0
BOOT_BLOCK_SECTOR_ADDR:   .byte 1

/******************************************************************************
 * Entry point
 ******************************************************************************/

realstart:

    /* Place the value 0x7c0 in the data segment register. Since we are in
       real mode, our data segment will start at physical address 0x7c00
       (0x7c0 * 16 = 0x7c00) which happens to be the address at which the
       BIOS has loaded our code. */
    mov $LO_BOOT_SEG, %ax
    mov %ax, %ds

    /* The BIOS places the boot drive number in DL before we take over.
       Let's save this value so we can read from the right drive later. */
    mov %dl, DRIVENUM

    /* Now, relocate our boot sector to the highest possible physical memory
       address. Note that the conventional memory stops at address 0xa0000
       (at which address the VGA memory starts) so our boot sector can
       theoretically be relocated to address 0x9fe00. However, I had weird
       errors with the BIOS int 13h in Bochs. Relocating the boot loader
       to a slightly lower address (0x9f000) solved that problem. I guess
       some small amount of memory below 0xa0000 is used by the BIOS.
       This new location corresponds to a segment value of 0x9f00 and
       an offset of 0. The copy is done by copying 256 2-byte words. */

    /* Initialize our loop counter to 256 */
    mov $(BLOCK_SIZE >> 1), %cx

    /* Reset the SI and DI registers. Note that we use XOR instead
       of MOV. Using XOR is faster and saves us 1 byte. */
    xor %si, %si
    xor %di, %di

    /* Our target segment is placed in ES */
    mov $HI_BOOT_SEG, %ax
    mov %ax, %es

    /* Make sure SI and DI are going to be incremented */
    cld

    /* Repeat the movsw instruction until the counter reaches 0 */
    rep
    movsw

    /* Finally, jump to the new location */
    ljmp $HI_BOOT_SEG, $newbootlocation

newbootlocation:

    /* Update the data segment register to reflect our new segment.
       Note that the AX register already contains the correct value. */
    mov %ax, %ds

    /* Setup a temporary stack for the boot sector and our kernel.
       Note that on x86 architectures, the stack grows downward,
       so this is indeed a very small stack (3KB) */
    xor %ax, %ax
    mov %ax, %ss
    mov $KERNEL_ADDR, %sp

    call clearscreen

/******************************************************************************
 * Determine how much memory is available on this system. Here, we use the
 * BIOS interrupt int 15h, ax=e801h. This function has been around since about
 * 1994, so all systems from after then up to now should have this function.
 * However, keep in mind that reliably figuring out how much memory is
 * installed on a system is next to impossible...
 * See http://www.pell.portland.or.us/~orc/Memory/
 * See http://www.osdev.org/osfaq2/index.php/How%20do%20I%20determine%20the%20amount%20of%20RAM%3F
 ******************************************************************************/

    mov $0xe801, %ax
    int $0x15
    jc error

    /* AX = extended memory between 1M and 16M, in K (max 3C00h=15 MB)
       BX = extended memory above 16M, in 64K blocks */
    mov %ax, memsize1
    mov %bx, memsize2

/******************************************************************************
 * Boot drive parameters auto-detection
 * Get the number of cylinders, heads and sectors per track on fd0 (A drive)
 * http://www.htl-steyr.ac.at/~morg/pcinfo/hardware/interrupts/inte6xyo.htm
 *
 * Note that this routine uses the old interrupt 13, function 8 ("Get Drive
 * Parameters"), which is limitied to a maximum cylinder count of 1023, so
 * the total sector count will be 16450560 (8 GB) for drives larger than 8GB.
 *
 * Note that it is possible to check whether the BIOS supports LBA. To do this,
 * refer to http://home.att.net/~rayknights/pc_boot/int13fun.htm. First, we
 * would have to check whether the BIOS interrupt 13 extensions are supported,
 * and then if extended read/write is supported. We don't do any of this here
 * since we start loading at block #2, and our kernel is really small...
 ******************************************************************************/

    /* 8 is the operation number. */
    mov $8, %ah

    /* Look at the boot drive. */
    mov DRIVENUM, %dl

    /* Issue the BIOS interrupt. */
    int $0x13

    jc error

    /* Write the values in memory. */
    mov %dh, HEADS
    mov %cl, SECTORS
    andb $0x3f, SECTORS
    mov %ch, CYLINDERS
    and $0xc0, %cx
    shl $2, %cx
    or %cx, CYLINDERS

/******************************************************************************
 * Load the kernel
 * http://www.htl-steyr.ac.at/~morg/pcinfo/hardware/interrupts/inte6xo0.htm
 ******************************************************************************/

    /* Store loadsize in DI, which is going to serve as our loop counter,
       because we will use loadsize again later. */
    movw loadsize, %di

    /* Check that our kernel is not too big. We can fit about 635 KB. */
    cmpw $MAX_KERNEL_SIZE, %di
    jg error

    /* Print the "Loading kernel" message. */
    PRINT(loadingkerstr)

    /* Make ES:BX point to the address where our kernel is to be loaded.
       We will increment ES and keep BX at 0 while loading sectors from
       the disk. This allows us to load a kernel bigger than 64KB. */
    mov $(KERNEL_ADDR >> 4), %ax
    mov %ax, %es
    xor %bx, %bx

    /* CH: low 8 bits of cylinder number
       CL: high 2 bits of cylinder number followed by sector number (6 bits)
       We start at sector number 2 (sector number 1 is for the boot sector)
       IMPORTANT NOTE: In the following code, we assume that we will not go
       beyond the 255th cylinder. Making this code load beyond the 255th
       cylinder is a little more complicated because of these 2 bits, which
       we will always consider to be null (no problem for a floppy disk) */
    mov BOOT_BLOCK_CYLINDER_ADDR, %ch
    mov BOOT_BLOCK_SECTOR_ADDR, %cl

    /* DH: head number. We start at head number 0.
       DL: drive number (0=1st floppy, 1=2nd floppy, 0x80=1st hdd, 0x81=2nd hdd) */
    mov BOOT_BLOCK_HEAD_ADDR, %dh
    mov DRIVENUM, %dl

loadkernel:

    /* Load the next sector:
         - Increment the sector number
         - If the sector number is <= SECTORS, then load the specified block
         - Otherwise, set the sector number to 0
         - Increment the head number
         - If the head number is <= HEADS, then load the specified block
         - Otherwise, set the head number to 0
         - Increment the cylinder number
         - If the cylinder number is <= CYLINDERS, then load the specified block
         - Otherwise, display an error
       Note that we calculate the address of the next sector before calling
       the BIOS interrupt. That is because of the initial condition: we have
       the CHS address of the boot block, not the CHS address of the block
       following it. We could have changed this, but the patchboot program
       does not need to know what the geometry of the disk is. It just reads
       entries in the partition table and patches our binary file... */

    /* The maximum number of retries when a read error occurs. This value
       goes in the stack because I don't have enough registers to hold it. */
    push $DISK_READ_RETRIES

    inc %cl
    cmp SECTORS, %cl
    jbe loadsector

    mov $1, %cl
    inc %dh
    cmp HEADS, %dh
    jbe loadsector

    xor %dh, %dh
    inc %ch
    cmp $0xff, %ch
    je error
    cmp CYLINDERS, %ch
    jbe loadsector
    jmp error

loadsector:

    /* AH: 0x02 = operation "read disk sectors"
       AL: 0x01 = number of sectors to read */
    mov $0x0201, %ax

    /* BX: offset address of dst buffer. Must be set every time since
       calls to print change the value of BX. */
    xor %bx, %bx

    /* Issue the BIOS interrupt. */
    int $0x13

    /* If CF=0, the read was successful. */
    jnc nextsector

    /* If a read error occurred, reset the drive (int 13h, AX=0)
       and try again, unless our error counter has reached 0. */
    pop %ax
    dec %ax
    jz error

    push %ax
    xor %ah, %ah
    int $0x13
    jmp loadsector

nextsector:

    /* Display a "." to show some progress. This is particularly useful
    when loading a large kernel from a slow floppy disk drive. */
    PRINT(loadingdotstr)

    /* Add 32 (0x20) to ES to skip 512 bytes of memory. */
    mov %es, %ax
    add $0x20, %ax
    mov %ax, %es

    /* Decrement the remaining number of sectors to load. */
    dec %di

    /* No more sectors to load? Then jump to the next set of instructions. */
    jnz loadkernel

    /* The kernel is now fully loaded in main memory.
       Stop the floppy drive motor. Here, we just read the content of the
       floppy drive controller register, set bits 4-7 to 0 (stop the motor
       on all drives) and store the new value to the controller register.
       Note that we have to set the port number via register DX because
       the port number is not in the range 0-255.
       See http://www.penguin.cz/~literakl/intel/i.html#IN
       See http://bochs.sourceforge.net/techspec/PORTS.LST */
    mov $0x3f2, %dx
    inb %dx, %al
    and $0xf, %al
    out %al, %dx

/******************************************************************************
 * Now, we have to enable the A20 address line, which is (or should be)
 * disabled by default. For a fascinating explanation on why we need to do
 * this, read http://en.wikibooks.org/wiki/X86_Assembly/16_32_and_64_Bits
 * There are several ways to enable the A20 address line. For a list, see
 * http://www.win.tue.nl/~aeb/linux/kbd/A20.html. For a list of I/O ports
 * and the possible commands one can issue on those ports, read
 * http://bochs.sourceforge.net/techspec/PORTS.LST
 ******************************************************************************/

    /* disable interrupts, or we're in for trouble. */
    cli

seta20.1:

    /* Read status of keyboard controller at port 0x64 */
    in $0x64, %al

    /* bit 1 = input buffer full (input 60/64 has data for 8042) */
    test $2, %al

    /* If bit 1 is non-zero, the keyboard controller is busy.
       Loop until it gets unbusy. */
    jnz seta20.1

    /* Writing 0xd1 to port 0x64 means that we are issuing a "write output port
       command" to the keyboard controller. The next byte written to 0x60 will be
       written to the 804x output port; the original IBM AT and many compatibles
       use bit 1 of the output port to control the A20 gate. */
    mov $0xd1, %al
    out %al, $0x64

seta20.2:

    /* Same story. Wait until the keyboard controller is not busy anymore. */
    in $0x64, %al
    test $2, %al
    jnz seta20.2

    /* Finally, write 0xdf to be written to the 804x output port
       This particular value enables the A20 address line. */
    mov $0xdf, %al
    out %al, $0x60

/******************************************************************************
 * Next step is to switch from 16-bit real mode to 32-bit protected mode.
 * For a good explanation on what these are and how to switch, read
 * http://en.wikibooks.org/wiki/X86_Assembly/Protected_Mode
 ******************************************************************************/

loadgdt:

    /* Load the Global Descriptor Table */
    lgdt gdtdesc

    /* Enable protected mode by setting bit 0 of register CR0 to 1
      See http://www.acm.uiuc.edu/sigops/roll_your_own/i386/regs.html */
    mov %cr0, %eax
    or  $1, %eax
    mov %eax, %cr0

    /* We are now in 32-bit protected mode */
    .code32

    /* Add the 32-bit prefix. This is essential. */
    .byte 0x66

    /* NEAR and SHORT jumps cause the IP to be updated while FAR jumps cause CS
       and IP to be updated. So here, we use a FAR jump to update CS and go to
       the next instruction. Note that it is not possible to set CS manually.
       Note the second part of the address is adjusted for the relocation. */
    ljmp $0x8, $(BOOTLOADER_FINAL_ADDR + pmode)

pmode:

    /* Set up the protected-mode data segment registers.
       Load 0x10 in DS. Note that 0x10 = 16, which is the index of our
       data segment descriptor in the GDT (8 bytes for the NULL descriptor,
       and 8 bytes for the code segment descriptor). From there, the x86
       chip is able to do the correct address translation. */
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    /* Setup the initial kernel stack (see simplix.lds) */
    mov $__boot_stack, %esp

/******************************************************************************
 * Compute the total amount of memory available and push it onto
 * the stack, so it is available to the operating system.
 ******************************************************************************/

    xor %eax, %eax
    mov (BOOTLOADER_FINAL_ADDR + memsize1), %ax
    shl $10, %eax
    mov (BOOTLOADER_FINAL_ADDR + memsize2), %bx
    shl $16, %ebx
    add %ebx, %eax
    add $0x100000, %eax
    push %eax

/******************************************************************************
 * That's it! We are now in 32-bit protected mode. The GDT is set to flat mode.
 * All interrupts are disabled, and we don't have an IDT yet. The PIC is not
 * programmed either. Let's call the kernel. Note that a far jump is required
 * here because we are jumping over more than 64KB of memory.
 ******************************************************************************/

    /* We cannot use 'call' (because we are jumping too far) and we are calling
       C-code which expects the stack to contain the return address first, then
       the function arguments, so push the return address onto the stack. */
    pushl spin

    /* And jump! */
    ljmp $0x8, $simplix_main

    /* The rest of the file contains routines used in 16-bit real mode */
    .code16

/******************************************************************************
 * Infinite loop reached when an error has been encountered and after the
 * appropriate message has been displayed.
 ******************************************************************************/

spin:

    jmp spin

/******************************************************************************
 * Error handling
 ******************************************************************************/

error:

    /* Clear the screen, display a generic error message and loop forever */
    call clearscreen
    PRINT(bootfailedstr)
    jmp spin

/******************************************************************************
 * print
 *
 * Writes a NULL terminated string in teletype mode.
 * You must set DS:SI to point at the string before calling this routine.
 * http://www.htl-steyr.ac.at/~morg/pcinfo/hardware/interrupts/inte6rao.htm
 ******************************************************************************/

print:

    /* Display parameter: white on black background */
    mov $0x0007, %bx

    /* 0x0e is the code for this specific operation of the interrupt 0x10 */
    mov $0x0e, %ah

    /* Load DS:SI in AL and increment SI */
    1: lodsb

    /* Stop if we have reached the end of the string (NULL character)
       Note: "1f" means: go to the first forward label "1:"
       See http://www.gnu.org/software/binutils/manual/gas-2.9.1/html_mono/as.html#SEC48 */
    or %al, %al
    jz 1f

    /* Issue the BIOS interrupt */
    int $0x10

    /* Move to the next character */
    jmp 1b
    1: ret

/******************************************************************************
 * clearscreen
 *
 * Clears the screen by setting the video mode (40x25 16 colors text mode)
 * http://www.htl-steyr.ac.at/~morg/pcinfo/hardware/interrupts/inte6l9s.htm
 ******************************************************************************/

clearscreen:

    mov $0x3, %ax
    int $0x10
    ret

/******************************************************************************
 * print_hex is for debugging purposes, and prints the word pointed
 * to by ss:bp in hexadecimal. Shamelessly stolen from LILO...
 ******************************************************************************/

#ifdef DEBUG

print_hex:
    mov $4, %cx
    mov (%bp), %dx
print_digit:
    rol $4, %dx
    mov $0xe0f, %ax
    and %dl, %al

    daa
    add $0xF0, %al
    adc $0x40, %al

    int $0x10
    loop print_digit
    ret

#endif

/******************************************************************************
 * String definitions
 ******************************************************************************/

bootfailedstr: .string "Boot failed"
loadingkerstr: .string "Loading kernel"
loadingdotstr: .string "."

/******************************************************************************
 * Data
 ******************************************************************************/

DRIVENUM:   .byte 0

CYLINDERS:  .word 0
HEADS:      .byte 0
SECTORS:    .byte 0

memsize1:   .word 0
memsize2:   .word 0

gdtdesc:

    /* The GDT is pointed to by a special register in the x86 chip, the GDT
       register, or simply GDTR. The GDTR is 48 bits long. The lower 16 bits
       tell the size of the GDT, and the upper 32 bits tell the location of
       the GDT in memory. */

    /* Size of GDT structure */
    .word gdtend - gdt - 1

    /* 32-bit linear address of GDT (after relocation of boot sector) */
    .long BOOTLOADER_FINAL_ADDR + gdt

gdt:

    /* See kernel/gdt.c for a description of this structure */

    /* The NULL descriptor */
    .long 0
    .long 0

    /* Code segment */
    .word 0xffff
    .word 0
    .byte 0
    .byte 0x9a # 10011010
    .byte 0xcf # 11001111
    .byte 0

    /* Data segment */
    .word 0xffff
    .word 0
    .byte 0
    .byte 0x92 # 10010010
    .byte 0xcf # 11001111
    .byte 0

gdtend:
