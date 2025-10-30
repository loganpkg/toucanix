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


#include "defs.h"
#include "address.h"
#include "assert.h"
#include "asm_lib.h"
#include "interrupt.h"
#include "allocator.h"
#include "paging.h"
#include "k_printf.h"
#include "process.h"
#include "screen.h"

extern char etext, edata, end;

void kernel_main(void)
{
    int r;
    uint64_t pml4_pa;

    init_idt();
    init_screen();

    (void) k_printf("Physical memory map:\n");
    r = print_memory_map_pa();
    assert(r == 0);

    (void) k_printf("etext: %lx\n", (unsigned long) &etext);
    (void) k_printf("edata: %lx\n", (unsigned long) &edata);
    (void) k_printf("end: %lx\n", (unsigned long) &end);

    r = init_free_physical_memory();
    assert(r == 0);

    r = check_physical_memory();
    assert(r == 0);

    pml4_pa = create_kernel_virtual_memory_space();
    assert(pml4_pa != 0);

    r = report_physical_memory();
    assert(r == 0);

    r = check_physical_memory();
    assert(r == 0);

    switch_pml4_pa(pml4_pa);

    (void) k_printf("Initialise process...\n");

    r = start_init_process();
    assert(r == 0);
}
