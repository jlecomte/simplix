/*===========================================================================
 *
 * kbd.c
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
 * Basic keyboard driver.
 *
 *===========================================================================*/

#include <simplix/io.h>
#include <simplix/proto.h>
#include <simplix/types.h>

#include <simplix/keymaps/us-std.h>
uint16_t *keymap = uskbd;

#define KBD_DATA_PORT 0x60

/* Forward declaration. */
static void handle_kbd_interrupt(uint32_t esp);

/*
 * Initializes the keyboard driver.
 */
void init_kbd(void)
{
    /* Set the IRQ handler for the keyboard */
    irq_set_handler(IRQ_KEYBOARD, handle_kbd_interrupt);

    /* Enable the keyboard IRQ */
    enable_irq_line(IRQ_KEYBOARD);
}

/*
 * Keyboard IRQ handler.
 */
static void handle_kbd_interrupt(uint32_t esp)
{
    int idx;
    uint8_t scancode;
    uint16_t key;
    static bool_t shiftkey = FALSE;

    /* Read the keyboard output buffer. Failing to do so would
       prevent us from receiving any subsequent interrupts... */
    scancode = inb(KBD_DATA_PORT);

    if (scancode >= 0x80) {
        /* A key has just been released */
        scancode -= 0x80;
        key = keymap[KEYMAP_COLS * scancode];
        if (key == LSHIFT || key == RSHIFT)
            shiftkey = FALSE;
    } else {
        /* A key has just been pressed */
        idx = KEYMAP_COLS * scancode;
        if (shiftkey)
            idx++;
        key = keymap[idx];
        if (key == LSHIFT || key == RSHIFT) {
            shiftkey = TRUE;
        } else if (key != 0) {
            // TO DO - TBD
            gfx_putchar(key);
        }
    }
}
