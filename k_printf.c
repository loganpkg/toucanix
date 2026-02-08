/*
 * Copyright (c) 2025, 2026 Logan Ryan McLintock. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


/*
 * Print function. The user version is generated from the kernel version,
 * so do not directly edit the user version.
 */


#include "stdarg.h"
#include "stdint.h"

#include "screen.h"
#include "defs.h"
/*+ #include "u_system_call.h" +*/


static int print_str(char *buf, int *used, const char *str)
{
    int ud = *used;
    char ch;

    while ((ch = *str++) != '\0') {
        if (ud == BUF_SIZE)
            return -1;

        *(buf + ud++) = ch;
    }

    *used = ud;
    return 0;
}


static int print_unsigned(char *buf, int *used, uint64_t x)
{
    char n[U64_MAX_DEC_DIGITS];
    int ud, i = 0;

    do {
        *(n + i++) = (char) ('0' + x % 10);
        x /= 10;
    } while (x);

    /* Reverse. */
    ud = *used;
    do {
        --i;
        if (ud == BUF_SIZE)
            return -1;

        *(buf + ud++) = *(n + i);
    } while (i);

    *used = ud;
    return 0;
}


static int print_hex(char *buf, int *used, uint64_t x)
{
    char *hex_map = "0123456789abcdef";
    char n[U64_MAX_HEX_DIGITS];
    int ud, i = 0;

    do {
        *(n + i++) = *(hex_map + x % 16);
        x /= 16;
    } while (x);

    /* Hex prefix backwards. */
    *(n + i++) = 'x';
    *(n + i++) = '0';

    /* Reverse. */
    ud = *used;
    do {
        --i;
        if (ud == BUF_SIZE)
            return -1;

        *(buf + ud++) = *(n + i);
    } while (i);

    *used = ud;
    return 0;
}


int k_printf(const char *format, ...)
{
    va_list a;
    char buf[BUF_SIZE];
    int used;

    char ch;
    char *str;
    uint64_t x;

    used = 0;

    va_start(a, format);

    while ((ch = *format++) != '\0') {
        if (ch == '%') {
            switch (*format++) {
            case 's':
                str = va_arg(a, char *);
                if (print_str(buf, &used, str) == -1)
                    return -1;

                break;
            case 'l':
                switch (*format++) {
                case 'u':
                    x = va_arg(a, uint64_t);
                    if (print_unsigned(buf, &used, x) == -1)
                        return -1;

                    break;
                case 'x':
                    x = va_arg(a, uint64_t);

                    if (print_hex(buf, &used, x) == -1)
                        return -1;

                    break;
                default:
                    return -1;
                }
                break;
            case '%':
                if (used == BUF_SIZE)
                    return -1;  /* Error */

                *(buf + used++) = '%';
                break;

            default:
                return -1;
            }
        } else {
            if (used == BUF_SIZE)
                return -1;      /* Error */

            *(buf + used++) = ch;
        }
    }

    va_end(a);

    write_to_screen(buf, used); /*-*/
    return used; /*-*/
    /*+ return u_system_write(STDOUT_FILENO, buf, (uint64_t) used); + */
}
