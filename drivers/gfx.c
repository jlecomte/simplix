/*===========================================================================
 *
 * gfx.c
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
 * A simple driver to access the VGA color text mode video memory, which is
 * mapped in RAM between 0xb8000 and 0xa0000. Nearly all display adapters
 * support this mode.
 *
 * See http://webster.cs.ucr.edu/AoA/DOS/ch23/CH23-1.html
 *
 *===========================================================================*/

#include <simplix/consts.h>
#include <simplix/io.h>
#include <simplix/types.h>

/* The address of the video memory */
#define VIDEO ((uint8_t *) 0xb8000)

static int offset;

/*
 * Clears the screen.
 */
void gfx_cls(void)
{
    int i;

    for (i = 0; i < SCREEN_ROWS * SCREEN_COLS; i++) {
        *(VIDEO + 2*i) = 0;
        *(VIDEO + 2*i + 1) = DEFAULT_TEXT_ATTR;
    }

    offset = 0;
}

/*
 * Writes the specified character, handle scrolling if needed,
 * and moves the cursor.
 */
void gfx_putchar(char c)
{
    int i;

    if (offset >= SCREEN_ROWS * SCREEN_COLS) {
        /* The screen needs to be scrolled 1 row */
        for (i=0 ; i<(SCREEN_ROWS-1)*(SCREEN_COLS - 1) ; i++) {
            *(VIDEO + 2*i) = *(VIDEO + 2*(SCREEN_COLS + i));
            *(VIDEO + 2*i + 1) = *(VIDEO + 2*(SCREEN_COLS + i) + 1);
        }
        /* Blank out the bottom-most row */
        for (i = (SCREEN_ROWS-1)*(SCREEN_COLS - 1);
             i < SCREEN_ROWS*(SCREEN_COLS-1); i++) {
            *(VIDEO + 2*i) = 0;
            *(VIDEO + 2*i + 1) = DEFAULT_TEXT_ATTR;
        }
        offset -= SCREEN_COLS;
    }

    if (c >= 32 && c <= 126) {
        /* This character is in the range of characters safe for printing */
        *(VIDEO + 2*offset) = c;
        *(VIDEO + 2*offset + 1) = DEFAULT_TEXT_ATTR;
        offset++;
    } else if (c == '\n') {
        offset = SCREEN_COLS * (1 + offset / SCREEN_COLS);
    }
}

/*
 * Writes the specified string, handle scrolling if needed,
 * and moves the cursor. The string must be NULL terminated.
 */
void gfx_putstring(char *s)
{
    char c;

    while ((c = *s++) != '\0')
        gfx_putchar(c);
}

/*
 * Returns the position of the cursor.
 */
int gfx_get_cursor_offset(void)
{
    return offset;
}

/*
 * Writes the specified character at the specified position.
 * Does NOT handle scrolling.
 * Does NOT move the cursor.
 */
void videomem_putchar(char c, int row, int col, uint8_t textattr)
{
    int idx = 2 * (row * SCREEN_COLS + col);
    *(VIDEO + idx) = c;
    *(VIDEO + idx + 1) = textattr;
}

/*
 * Writes the specified string starting at the specified position.
 * The string must be NULL terminated.
 * Does NOT handle scrolling.
 * Does NOT move the cursor.
 */
void videomem_putstring(char *s, int row, int col, uint8_t textattr)
{
    char c;
    int i = row * SCREEN_COLS + col;

    while ((c = *s++) != '\0') {
        if (i < 0 || i >= SCREEN_ROWS * SCREEN_COLS)
            break;
        *(VIDEO + 2*i) = c;
        *(VIDEO + 2*i + 1) = textattr;
        i++;
    }
}

/*
 * Initializes the text-mode video driver.
 */
void init_gfx(void)
{
    offset = 0;

    gfx_cls();

    /* Hide cursor */
    outb(0x3d4, 0x0a);
    outb(0x3d5, 1 << 5);
}
