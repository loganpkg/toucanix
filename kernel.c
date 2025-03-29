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
    uint64_t pml4_pa_A, pml4_pa_B;

    init_idt();
    init_screen();
    (void) print_memory_map_pa();
    printf("etext: %lx\n", (unsigned long) &etext);
    printf("edata: %lx\n", (unsigned long) &edata);
    printf("end: %lx\n", (unsigned long) &end);


    (void) init_free_physical_memory();

    check_physical_memory();

    pml4_pa_A = create_kernel_virtual_memory_space();
    assert(pml4_pa_A != 0);

    report_physical_memory();
    check_physical_memory();
    switch_pml4_pa(pml4_pa_A);

    pml4_pa_B = create_kernel_virtual_memory_space();
    assert(pml4_pa_B != 0);

    report_physical_memory();
    check_physical_memory();
    switch_pml4_pa(pml4_pa_B);

    free_memory_space(pml4_pa_A);

    report_physical_memory();
    check_physical_memory();

    (void) printf("Done\n");
}
