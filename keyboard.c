/*
 * Copyright (c) 2026 Logan Ryan McLintock. All rights reserved.
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

#include "keyboard.h"
#include "circular_buffer.h"
#include "defs.h"
#include "interrupt.h"
#include "k_printf.h"

#define PS2_DATA_PORT 0x60

#define KEY_RELEASE 0x80

#define ESC             '\x1B'
#define LEFT_SHIFT_KEY  '\x2A'
#define RIGHT_SHIFT_KEY '\x36'
#define CONTROL_KEY     '\x1D'
#define CAPS_LOCK_KEY   '\x3A'
#define SPECIAL         0xE0

const char scan_code_to_ascii[U8_MAX + 1] = {
    '\0',
    ESC,
    '1',
    '2',
    '3',
    '4',
    '5',
    '6',
    '7',
    '8',
    '9',
    '0',
    '-',
    '=',
    '\b',
    '\t',
    'q',
    'w',
    'e',
    'r',
    't',
    'y',
    'u',
    'i',
    'o',
    'p',
    '[',
    ']',
    '\n',
    '\0',
    'a',
    's',
    'd',
    'f',
    'g',
    'h',
    'j',
    'k',
    'l',
    ';',
    '\'',
    '`',
    '\0',
    '\\',
    'z',
    'x',
    'c',
    'v',
    'b',
    'n',
    'm',
    ',',
    '.',
    '/',
    '\0',
    '\0',
    '\0',
    ' ',
    '\0',
    F1_KEY,
    F2_KEY,
    F3_KEY,
    F4_KEY,
    F5_KEY,
    F6_KEY,
    F7_KEY,
    F8_KEY,
    F9_KEY,
    F10_KEY,
    '\0',
    '\0',
    HOME_KEY,
    UP_KEY,
    PAGE_UP_KEY,
    '\0',
    LEFT_KEY,
    '\0',
    RIGHT_KEY,
    '\0',
    END_KEY,
    DOWN_KEY,
    PAGE_DOWN_KEY,
    INSERT_KEY,
    DELETE_KEY,
    '\0',
    '\0',
    '\0',
    F11_KEY,
    F12_KEY,
};

const char shift_scan_code_to_ascii[U8_MAX + 1] = {
    '\0',
    ESC,
    '!',
    '@',
    '#',
    '$',
    '%',
    '^',
    '&',
    '*',
    '(',
    ')',
    '_',
    '+',
    '\b',
    '\t',
    'Q',
    'W',
    'E',
    'R',
    'T',
    'Y',
    'U',
    'I',
    'O',
    'P',
    '{',
    '}',
    '\n',
    '\0',
    'A',
    'S',
    'D',
    'F',
    'G',
    'H',
    'J',
    'K',
    'L',
    ':',
    '"',
    '~',
    '\0',
    '|',
    'Z',
    'X',
    'C',
    'V',
    'B',
    'N',
    'M',
    '<',
    '>',
    '?',
    '\0',
    '\0',
    '\0',
    ' ',
    '\0',
    F1_KEY,
    F2_KEY,
    F3_KEY,
    F4_KEY,
    F5_KEY,
    F6_KEY,
    F7_KEY,
    F8_KEY,
    F9_KEY,
    F10_KEY,
    '\0',
    '\0',
    HOME_KEY,
    UP_KEY,
    PAGE_UP_KEY,
    '\0',
    LEFT_KEY,
    '\0',
    RIGHT_KEY,
    '\0',
    END_KEY,
    DOWN_KEY,
    PAGE_DOWN_KEY,
    INSERT_KEY,
    DELETE_KEY,
    '\0',
    '\0',
    '\0',
    F11_KEY,
    F12_KEY,
};

#if !DEBUG_SCAN_CODES
static int shift_on = 0;
static int control_on = 0;
static int caps_lock_on = 0;

struct circular_buffer cb;
#endif

#if DEBUG_SCAN_CODES
void keyboard(void)
{
    unsigned char u;
    u = read_byte(PS2_DATA_PORT);
    k_printf("%lx ", (uint64_t) u);
}

#else

void keyboard(void)
{
    unsigned char u;
    char ch;

    u = read_byte(PS2_DATA_PORT);

    if (u == LEFT_SHIFT_KEY || u == RIGHT_SHIFT_KEY) {
        shift_on = 1;
        return;
    }

    if (u == CONTROL_KEY) {
        control_on = 1;
        return;
    }

    if (u == CAPS_LOCK_KEY) {
        caps_lock_on = !caps_lock_on; /* Toggle. */
        return;
    }

    if (u == SPECIAL)
        return;

    if (u == (LEFT_SHIFT_KEY | KEY_RELEASE)
        || u == (RIGHT_SHIFT_KEY | KEY_RELEASE)) {
        shift_on = 0;
        return;
    }

    if (u == (CONTROL_KEY | KEY_RELEASE)) {
        control_on = 0;
        return;
    }

    if (u & KEY_RELEASE)
        return;

    if (shift_on || control_on)
        ch = shift_scan_code_to_ascii[u];
    else
        ch = scan_code_to_ascii[u];

    if (control_on) {
        if ((ch >= '@' && ch <= '_') || ch == '?')
            ch = ch ^ 1 << 6; /* Toggle bit 6. */
        else
            return;
    }

    if (caps_lock_on && ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')))
        ch = ch ^ 1 << 5; /* Toggle bit 5. */

    if (ch)
        k_printf("%c", ch);
}

#endif
