OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH("i386")

SECTIONS
{
    /* Start at address 0 so the boot sector can easily be relocated. */

    .bootsect 0 : {
        /* The code for the boot sector goes here */
        *(.bootsect)

        /* The loadsize symbol contains the size of the area (in number of
           512-byte blocks) that the boot sector should copy from the disk. */
        loadsize = .;
        SHORT( ( __e_load - __b_load + 511 ) >> 9 );
        /* ---> This is equivalent to ceil( ( __e_load - __b_load ) / 512 ) */

        /* At offsets 511 and 512, we set the boot sector signature (0xAA55) */
        . = 0x1fe;
        SHORT(0xAA55);
    }

    /* This is the address at which the kernel gets loaded. Don't change this
       value without changing the macro KERNEL_ADDR in bootsect.S */
    . = 0x1000;

    /* We take note of the start of the kernel */
    __b_kernel = .;

    /* We take note of the beginning of the data to load. */
    __b_load = .;

    .text . : {
        *(.text)
    }

    /* We take note of the end of the kernel's text section, which we align on
       a page boundary. */

    . = ALIGN(4096);

    __e_text = .;

    .data . : {
        *(.data)
    }

    .rodata . : {
        *(.rodata)
    }

    /* We take note of the end of the data to load.
       The bss section is not included since it uses 0 bytes on disk. */

    __e_load = .;

    .bss : {
        *(.bss)
    }

    /* Allocate some memory for the stack used by the kernel during boot time
       (aligned on a page boundary) This stack later becomes the user space
       stack for the idle task. */

    . = ALIGN(4096);

    . += 0x1000;
    __boot_stack = .;
    __idle_ustack = .;

    /* We take note of the end of the kernel */
    __e_kernel = .;

    /* We don't care about the following sections generated by gcc. */

    /DISCARD/ : {
        *(.note*)
        *(.indent*)
        *(.comment*)
        *(.stab*)
        *(.stabstr*)
    }
}

/* Define the entry point just to get rid of an ld warning. */
ENTRY(simplix_main)
