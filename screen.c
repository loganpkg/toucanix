/*
 * Copyright (c) 2025 Logan Ryan McLintock
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */


#include "stdint.h"

#include "defs.h"
#include "address.h"
#include "asm_lib.h"
#include "screen.h"


#define ROW_VA PRINT_VA
#define COL_VA (PRINT_VA + 4)

#define text_ptr(r, c) ((char *) VIDEO_VA + (r) * BYTES_PER_LINE \
    + (c) * BYTES_PER_SCREEN_CHAR)

#define colour_ptr(r, c) ((uint8_t *) text_ptr((r), (c)) + 1)


static uint64_t row = 0, col = 0;


void init_screen(void)
{
    row = *(uint32_t *) ROW_VA;
    col = *(uint32_t *) COL_VA;
}


void write_to_screen(char *buf, int s)
{
    int i;
    char ch;

    for (i = 0; i < s; ++i) {
        if (col == SCREEN_WIDTH) {
            /* Wrap line. */
            ++row;
            col = 0;
        }
        if (row == SCREEN_HEIGHT) {
            /* Scroll up a line. */
            memmove((void *) VIDEO_VA,
                    (const void *) (VIDEO_VA + BYTES_PER_LINE),
                    (SCREEN_HEIGHT - 1) * BYTES_PER_LINE);
            --row;
            /* Clear last row. */
            memset((void *) (VIDEO_VA + (SCREEN_HEIGHT - 1) * BYTES_PER_LINE),
                   0, BYTES_PER_LINE);
        }
        ch = *(buf + i);

        if (ch == '\n') {
            ++row;
            col = 0;
        } else {
            *text_ptr(row, col) = ch;
            *colour_ptr(row, col) = DEFAULT_COLOUR;
            ++col;
        }
    }
}
