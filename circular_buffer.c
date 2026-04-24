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

/* Circular buffer using static memory. */

#include "circular_buffer.h"

#ifdef TOUCANIX
#include "k_printf.h"
#include "stdint.h"
#else
#include <stdint.h>
#include <stdio.h>
#endif

void dump_cb(struct circular_buffer *cb)
{
    int i;
    k_printf("============\n");
    k_printf("start: %ld\n", (long int) cb->start);
    k_printf("end: %ld\n", (long int) cb->end);
    k_printf("used: %ld\n", (long int) cb->used);
    k_printf("CIRCULAR_BUFFER_SIZE: %ld\n", (long int) CIRCULAR_BUFFER_SIZE);
    k_printf("Mem:");
    if (cb->start < cb->end) {
        for (i = cb->start; i < cb->end; ++i) k_printf(" %c", cb->mem[i]);
    } else if (cb->used) {
        for (i = cb->start; i < CIRCULAR_BUFFER_SIZE; ++i)
            k_printf(" %c", cb->mem[i]);

        for (i = 0; i < cb->end; ++i) k_printf(" %c", cb->mem[i]);
    }
    k_printf("\n============\n");
}

void init_cb(struct circular_buffer *cb)
{
    cb->start = 0;
    cb->end = 0;
    cb->used = 0;
}

int write_to_cb(struct circular_buffer *cb, unsigned char u)
{
    if (cb->used == CIRCULAR_BUFFER_SIZE)
        return 1; /* Full. */

    cb->mem[cb->end] = u;
    if (++cb->end == CIRCULAR_BUFFER_SIZE) /* Wrap. */
        cb->end = 0;

    ++cb->used;
    return 0;
}

int read_from_cb(struct circular_buffer *cb, unsigned char *u)
{
    if (!cb->used)
        return 1; /* Empty. Nothing to read. */

    *u = cb->mem[cb->start];
    if (++cb->start == CIRCULAR_BUFFER_SIZE) /* Wrap. */
        cb->start = 0;

    --cb->used;
    return 0;
}
