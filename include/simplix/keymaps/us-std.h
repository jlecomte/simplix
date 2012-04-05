/*===========================================================================
 *
 * us-std.h
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
 *===========================================================================*
 *
 * Scancode table for a US qwerty keyboard.
 *
 *===========================================================================*/

#ifndef _SIMPLIX_US_STD_H_
#define _SIMPLIX_US_STD_H_

#define SCAN_CODES_COUNT 128
#define KEYMAP_COLS       2

uint16_t uskbd[SCAN_CODES_COUNT * KEYMAP_COLS] = {

/*==========================================
    scancode       !shift           shift
  ==========================================*/

    /*  0 */              0,                  0,
    /*  1 */         ESCAPE,     SHIFT + ESCAPE,
    /*  2 */            '1',                '!',
    /*  3 */            '2',                '@',
    /*  4 */            '3',                '#',
    /*  5 */            '4',                '$',
    /*  6 */            '5',                '%',
    /*  7 */            '6',                '^',
    /*  8 */            '7',                '&',
    /*  9 */            '8',                '*',
    /* 10 */            '9',                '(',
    /* 11 */            '0',                ')',
    /* 12 */            '-',                '_',
    /* 13 */            '=',                '+',
    /* 14 */           '\b',       SHIFT + '\b',
    /* 15 */           '\t',       SHIFT + '\t',
    /* 16 */            'q',                'Q',
    /* 17 */            'w',                'W',
    /* 18 */            'e',                'E',
    /* 19 */            'r',                'R',
    /* 20 */            't',                'T',
    /* 21 */            'y',                'Y',
    /* 22 */            'u',                'U',
    /* 23 */            'i',                'I',
    /* 24 */            'o',                'O',
    /* 25 */            'p',                'P',
    /* 26 */            '[',                '{',
    /* 27 */            ']',                '}',
    /* 28 */           '\n',       SHIFT + '\n',
    /* 29 */           CTRL,       SHIFT + CTRL,
    /* 30 */            'a',                'A',
    /* 31 */            's',                'S',
    /* 32 */            'd',                'D',
    /* 33 */            'f',                'F',
    /* 34 */            'g',                'G',
    /* 35 */            'h',                'H',
    /* 36 */            'j',                'J',
    /* 37 */            'k',                'K',
    /* 38 */            'l',                'L',
    /* 39 */            ';',                ':',
    /* 40 */           '\'',                '"',
    /* 41 */            '`',                '~',
    /* 42 */         LSHIFT,     SHIFT + LSHIFT,
    /* 43 */           '\\',                '|',
    /* 44 */            'z',                'Z',
    /* 45 */            'x',                'X',
    /* 46 */            'c',                'C',
    /* 47 */            'v',                'V',
    /* 48 */            'b',                'B',
    /* 49 */            'n',                'N',
    /* 50 */            'm',                'M',
    /* 51 */            ',',                '<',
    /* 52 */            '.',                '>',
    /* 53 */            '/',                '?',
    /* 54 */         RSHIFT,     SHIFT + RSHIFT,
    /* 55 */            '*',                '*',
    /* 56 */            ALT,        SHIFT + ALT,
    /* 57 */            ' ',                ' ',
    /* 58 */      CAPS_LOCK,  SHIFT + CAPS_LOCK,
    /* 59 */             F1,        SHIFT +  F1,
    /* 60 */             F2,        SHIFT +  F2,
    /* 61 */             F3,        SHIFT +  F3,
    /* 62 */             F4,        SHIFT +  F4,
    /* 63 */             F5,        SHIFT +  F5,
    /* 64 */             F6,        SHIFT +  F6,
    /* 65 */             F7,        SHIFT +  F7,
    /* 66 */             F8,        SHIFT +  F8,
    /* 67 */             F9,        SHIFT +  F9,
    /* 68 */            F10,        SHIFT + F10,
    /* 69 */       NUM_LOCK,   SHIFT + NUM_LOCK,
    /* 70 */       SCR_LOCK,   SHIFT + SCR_LOCK,
    /* 71 */           HOME,       SHIFT + HOME,
    /* 72 */             UP,         SHIFT + UP,
    /* 73 */           PGUP,       SHIFT + PGUP,
    /* 74 */            '-',                '-',
    /* 75 */           LEFT,       SHIFT + LEFT,
    /* 76 */         CENTER,     SHIFT + CENTER,
    /* 77 */          RIGHT,      SHIFT + RIGHT,
    /* 78 */            '+',                '+',
    /* 79 */            END,        SHIFT + END,
    /* 80 */           DOWN,       SHIFT + DOWN,
    /* 81 */           PGDN,       SHIFT + PGDN,
    /* 82 */         INSERT,     SHIFT + INSERT,
    /* 83 */         DELETE,     SHIFT + DELETE,
    /* 84 */              0,                  0,
    /* 85 */              0,                  0,
    /* 86 */              0,                  0,
    /* 87 */            F11,        SHIFT + F11,
    /* 88 */            F12,        SHIFT + F12,
    /* 89 */             0,                   0,
    /* All other keys are undefined */
};

#endif /* _SIMPLIX_US_STD_H_ */
