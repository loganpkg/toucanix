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

#include "stdint.h"

#include "address.h"
#include "asm_lib.h"
#include "defs.h"
#include "screen.h"

#define ROW_VA PRINT_VA
#define COL_VA (PRINT_VA + 4)

#define text_ptr(r, c)                                                        \
    ((unsigned char *) VIDEO_VA + (r) * BYTES_PER_LINE                        \
        + (c) * BYTES_PER_SCREEN_CHAR)

#define colour_ptr(r, c) ((uint8_t *) text_ptr((r), (c)) + 1)

static uint64_t row = 0, col = 0;

void init_screen(void)
{
    row = *(uint32_t *) ROW_VA;
    col = *(uint32_t *) COL_VA;
}

void wrap_screen(void)
{
    /* Wrap if off the screen. */
    if (col == SCREEN_WIDTH) {
        /* Wrap line. */
        ++row;
        col = 0;
    }
    if (row == SCREEN_HEIGHT) {
        /* Scroll up a line. */
        memmove((void *) VIDEO_VA, (const void *) (VIDEO_VA + BYTES_PER_LINE),
            (SCREEN_HEIGHT - 1) * BYTES_PER_LINE);
        --row;
        /* Clear last row. */
        memset((void *) (VIDEO_VA + (SCREEN_HEIGHT - 1) * BYTES_PER_LINE), 0,
            BYTES_PER_LINE);
    }
}

void put_char_to_screen(char ch)
{
    char *hex_map = "0123456789ABCDEF";
    unsigned char u;
    int i;

    u = (unsigned char) ch;

    if (u == '\n') {
        ++row;
        col = 0;
        wrap_screen();
    } else if (u == '\t') {
        for (i = 0; i < TAB_SIZE; ++i) {
            *text_ptr(row, col) = ' ';
            *colour_ptr(row, col) = DEFAULT_COLOUR;
            ++col;
            wrap_screen();
        }
    } else if (u <= 0x1F || u == 0x7F) {
        /* Control character. Print in caret notation. */
        *text_ptr(row, col) = '^';
        *colour_ptr(row, col) = RED;
        ++col;
        wrap_screen();

        *text_ptr(row, col) = u ^ 1 << 6; /* Toggle bit 6. */
        *colour_ptr(row, col) = RED;
        ++col;
        wrap_screen();
    } else if (u >= 0x20 && u <= 0x7E) {
        /* Printable character (including space). */
        *text_ptr(row, col) = u;
        *colour_ptr(row, col) = DEFAULT_COLOUR;
        ++col;
        wrap_screen();
    } else {
        /* Print as hex. */
        *text_ptr(row, col) = '\\';
        *colour_ptr(row, col) = RED;
        ++col;
        wrap_screen();

        *text_ptr(row, col) = 'x';
        *colour_ptr(row, col) = RED;
        ++col;
        wrap_screen();

        *text_ptr(row, col) = hex_map[u / 16];
        *colour_ptr(row, col) = RED;
        ++col;
        wrap_screen();

        *text_ptr(row, col) = hex_map[u % 16];
        *colour_ptr(row, col) = RED;
        ++col;
        wrap_screen();
    }
}

void write_to_screen(char *buf, int s)
{
    int i;

    for (i = 0; i < s; ++i) put_char_to_screen(*(buf + i));
}
