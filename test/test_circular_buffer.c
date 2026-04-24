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

/* Test circular buffer. */

#include "../circular_buffer.h"
#include <stdio.h>

int main(void)
{
    unsigned char u;
    struct circular_buffer cb;

    init_cb(&cb);

    u = 'A';
    write_to_cb(&cb, u);
    dump_cb(&cb);

    u = 'B';
    write_to_cb(&cb, u);
    dump_cb(&cb);

    u = 'C';
    write_to_cb(&cb, u);
    dump_cb(&cb);

    u = 'D';
    write_to_cb(&cb, u);
    dump_cb(&cb);

    u = 'E';
    write_to_cb(&cb, u);
    dump_cb(&cb);

    read_from_cb(&cb, &u);
    printf("Read: %c\n", u);
    dump_cb(&cb);

    read_from_cb(&cb, &u);
    printf("Read: %c\n", u);
    dump_cb(&cb);

    u = 'F';
    write_to_cb(&cb, u);
    dump_cb(&cb);

    u = 'G';
    write_to_cb(&cb, u);
    dump_cb(&cb);

    u = 'H';
    write_to_cb(&cb, u);
    dump_cb(&cb);

    read_from_cb(&cb, &u);
    printf("Read: %c\n", u);
    dump_cb(&cb);

    read_from_cb(&cb, &u);
    printf("Read: %c\n", u);
    dump_cb(&cb);

    read_from_cb(&cb, &u);
    printf("Read: %c\n", u);
    dump_cb(&cb);

    read_from_cb(&cb, &u);
    printf("Read: %c\n", u);
    dump_cb(&cb);

    return 0;
}
