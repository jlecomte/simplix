/*===========================================================================
 *
 * string.c
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
 * String/memory manipulation routines. These are definitely not optimized
 * (in a real world system, they would be written in assembly code) but they
 * are easy to understand, and portable.
 *
 *===========================================================================*/

#include <string.h>
#include <vararg.h>

#include <simplix/types.h>

void *memset(void *s, int c, size_t size)
{
    char *p = (char *) s;
    while (size-- > 0)
        *p++ = c;
    return s;
}

void *memcpy(void *dest, const void *src, size_t n)
{
    char *s = (char *) src;
    char *d = (char *) dest;
    while (n-- > 0)
        *d++ = *s++;
    return dest;
}

int vsnprintf(char *str, size_t size, const char *format, va_list ap)
{
    char c;
    int i = 0, count = 0, minwidth = -1, zeropad = FALSE;

    #define OUTPUTCHAR(c)       \
    do {                        \
        *str++ = (c);           \
        count++;                \
        if (count >= size-1)    \
            goto end;           \
    } while (0)

    while ((c = format[i++]) != '\0') {

        if (minwidth == -1) {
            if (c != '%') {
                OUTPUTCHAR(c);
                continue;
            }
            c = format[i++];
        }

        switch (c) {

            case '%':
                /* Escaped '%' */
                OUTPUTCHAR('%');
                OUTPUTCHAR('%');
                break;

            case 'c':
                /* Single character. */
                c = va_arg(ap, char);
                OUTPUTCHAR(c);
                break;

            case 's': {
                /* String. */
                char *s = va_arg(ap, char *);
                while (*s != '\0')
                    OUTPUTCHAR(*s++);
                break;
            }

            case '.': {
                /* minwidth maybe? */
                int j = i;
                minwidth = 0;
                zeropad = FALSE;
                while ((c = format[j++]) != '\0') {
                    if (c >= '0' && c <= '9') {
                        minwidth = 10 * minwidth + c - '0';
                        if (c == '0' && j == i+1)
                            zeropad = TRUE;
                    } else if (c == 'u' || c == 'd' || c == 'x' || c == 'X') {
                        i = j-1;
                        break;
                    } else {
                        break;
                    }
                }
                if (minwidth == -1) {
                    /* Invalid minwidth. Output original string. */
                    OUTPUTCHAR('%');
                    OUTPUTCHAR('.');
                }
                break;
            }

            case 'u': {
                /* Unsigned decimal. */
                int i = 0;
                int digits[10];
                unsigned int n = va_arg(ap, int);
                do {
                    digits[i++] = n % 10;
                    n /= 10;
                } while (n > 0);
                while (--minwidth >= i)
                    OUTPUTCHAR(zeropad ? '0' : ' ');
                while (--i >= 0)
                    OUTPUTCHAR('0' + digits[i]);
                minwidth = -1;
                break;
            }

            case 'd': {
                /* Signed decimal. */
                int i = 0;
                int digits[10];
                int n = va_arg(ap, int);
                if (n < 0) {
                    OUTPUTCHAR('-');
                    n = -n;
                }
                do {
                    digits[i++] = n % 10;
                    n /= 10;
                } while (n > 0);
                while (--minwidth >= i)
                    OUTPUTCHAR(zeropad ? '0' : ' ');
                while (--i >= 0)
                    OUTPUTCHAR('0' + digits[i]);
                minwidth = -1;
                break;
            }

            case 'x':
            case 'X': {
                /* Unsigned hexadecimal. */
                int i = 0;
                int digits[8];
                unsigned int n = va_arg(ap, int);
                do {
                    digits[i++] = n % 16;
                    n /= 16;
                } while (n > 0);
                while (--minwidth >= i)
                    OUTPUTCHAR(zeropad ? '0' : ' ');
                while (--i >= 0) {
                    if (digits[i] < 10) {
                        OUTPUTCHAR('0' + digits[i]);
                    } else if (c == 'x') {
                        OUTPUTCHAR('a' + digits[i] - 10);
                    } else {
                        OUTPUTCHAR('A' + digits[i] - 10);
                    }
                }
                minwidth = -1;
                break;
            }

            default:
                /* We don't support this type of conversion. */
                OUTPUTCHAR('%');
                OUTPUTCHAR(c);
                break;
        }
    }

end:

    *str = '\0';
    count++;

    return count;
}

int snprintf(char *str, size_t size, const char *format, ...)
{
    int len;
    va_list ap;

    va_start(ap, format);
    len = vsnprintf(str, size, format, ap);

    return len;
}

size_t strlen(const char *str)
{
    const char *s = str;
    while (*s++ != '\0') ;
    return s - str;
}

char *strcpy(char *dest, const char *src)
{
    char *dst = dest;
    while (*src != '\0')
        *dst++ = *src++;
    *dst = '\0';
    return dest;
}

char *strncpy(char *dest, const char *src, size_t n)
{
    size_t i;
    for (i = 0 ; i < n && src[i] != '\0' ; i++)
        dest[i] = src[i];
    for ( ; i < n ; i++)
        dest[i] = '\0';
    return dest;
}

int strcmp(const char *s1, const char *s2)
{
    while (*s1 == *s2++)
        if (*s1++ == '\0')
            return 0;
    return *s1 - *(s2 - 1);
}
