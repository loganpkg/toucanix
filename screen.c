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


#include "asm_lib.h"
#include "screen.h"


#define text_ptr(r, c) ((char *) VIDEO_ADDRESS + (r) * SCREEN_WIDTH * 2 \
    + (c) * 2)

#define colour_ptr(r, c) ((unsigned char *) text_ptr((r), (c)) + 1)


/* Note that row index starts from zero, not one. */
static size_t row = 0, col = 0;


void write_to_screen(char *buf, int s, unsigned char colour)
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
            memmove((char *) VIDEO_ADDRESS,
                    (char *) VIDEO_ADDRESS + SCREEN_WIDTH * 2,
                    (SCREEN_HEIGHT - 1) * SCREEN_WIDTH * 2);
            --row;
            /* Clear last row. */
            memset((char *) VIDEO_ADDRESS +
                   (SCREEN_HEIGHT - 1) * SCREEN_WIDTH * 2, '\0',
                   SCREEN_WIDTH * 2);
        }
        ch = *(buf + i);

        if (ch == '\n') {
            ++row;
            col = 0;
        } else {
            *text_ptr(row, col) = ch;
            *colour_ptr(row, col) = colour;
            ++col;
        }
    }
}
