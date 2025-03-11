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


#include "address.h"
#include "assert.h"
#include "asm_lib.h"
#include "interrupt.h"
#include "memory.h"
#include "printf.h"
#include "screen.h"

extern char etext, edata, end;

void kernel_main(void)
{
    int r;

    init_idt();
    init_screen();
    (void) print_memory_map_pa();
    printf("etext: %lx\n", (unsigned long) &etext);
    printf("edata: %lx\n", (unsigned long) &edata);
    printf("end: %lx\n", (unsigned long) &end);



    (void) collect_free_memory();

    (void) printf("memcmp: %lx\n", (unsigned long) memcmp("abz", "abc", 4));




    r = init_kernel_virtual_memory_space();
    assert(r == 0);



    (void) printf("cool\n");
}
