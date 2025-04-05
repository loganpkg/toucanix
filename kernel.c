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
#include "process.h"
#include "screen.h"

extern char etext, edata, end;

void kernel_main(void)
{
    int r;
    uint64_t pml4_pa_A, pml4_pa_B;

    init_idt();
    init_screen();

    r = print_memory_map_pa();
    assert(r == 0);

    printf("etext: %lx\n", (unsigned long) &etext);
    printf("edata: %lx\n", (unsigned long) &edata);
    printf("end: %lx\n", (unsigned long) &end);

    r = init_free_physical_memory();
    assert(r == 0);

    r = check_physical_memory();
    assert(r == 0);

    pml4_pa_A = create_kernel_virtual_memory_space();
    assert(pml4_pa_A != 0);

    r = report_physical_memory();
    assert(r == 0);

    r = check_physical_memory();
    assert(r == 0);

    switch_pml4_pa(pml4_pa_A);

    pml4_pa_B = create_kernel_virtual_memory_space();
    assert(pml4_pa_B != 0);

    r = report_physical_memory();
    assert(r == 0);

    r = check_physical_memory();
    assert(r == 0);

    switch_pml4_pa(pml4_pa_B);

    free_4_level_paging(pml4_pa_A);

    r = report_physical_memory();
    assert(r == 0);

    r = check_physical_memory();
    assert(r == 0);

    printf("init process...\n");

    r = start_init_process();
    assert(r == 0);
}
