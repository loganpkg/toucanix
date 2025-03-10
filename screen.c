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

#include "address.h"
#include "asm_lib.h"
#include "screen.h"


#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25
#define BYTES_PER_SCREEN_CHAR 2
#define BYTES_PER_LINE (SCREEN_WIDTH * BYTES_PER_SCREEN_CHAR)


#define MBR_ADDRESS 0x7c00
#define MBR_SECTOR 1
#define BYTES_PER_SECTOR 512
#define PRINT_ADDRESS (MBR_ADDRESS + (MBR_SECTOR * BYTES_PER_SECTOR))
#define PRINT_VIRTUAL_ADDRESS (KERNEL_SPACE_VIRTUAL_ADDRESS + PRINT_ADDRESS)
#define ROW PRINT_VIRTUAL_ADDRESS
#define COL (PRINT_VIRTUAL_ADDRESS + 4)


#define text_ptr(r, c) ((char *) VIDEO_VIRTUAL_ADDRESS + (r) * BYTES_PER_LINE \
    + (c) * BYTES_PER_SCREEN_CHAR)

#define colour_ptr(r, c) ((uint8_t *) text_ptr((r), (c)) + 1)


static uint64_t row = 0, col = 0;


void init_screen(void)
{
    row = *(uint32_t *) ROW;
    col = *(uint32_t *) COL;
}


void write_to_screen(char *buf, int s, uint8_t colour)
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
            memmove((void *) VIDEO_VIRTUAL_ADDRESS,
                    (const void *) (VIDEO_VIRTUAL_ADDRESS +
                                    BYTES_PER_LINE),
                    (SCREEN_HEIGHT - 1) * BYTES_PER_LINE);
            --row;
            /* Clear last row. */
            memset((void *) (VIDEO_VIRTUAL_ADDRESS
                             + (SCREEN_HEIGHT - 1) * BYTES_PER_LINE), 0,
                   BYTES_PER_LINE);
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
