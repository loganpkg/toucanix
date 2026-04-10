/*
 * Copyright (c) 2025 Logan Ryan McLintock. All rights reserved.
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

#include "address.h"
#include "allocator.h"
#include "asm_lib.h"
#include "defs.h"
#include "interrupt.h"
#include "k_printf.h"
#include "paging.h"
#include "process.h"
#include "screen.h"
#include "stop.h"

extern char etext, edata, end;

void kernel_main(void)
{
    uint64_t pml4_pa;

    init_idt();
    init_screen();

    (void) k_printf("Physical memory map:\n");
    stop(print_memory_map_pa());

    (void) k_printf("etext: %lx\n", (unsigned long) &etext);
    (void) k_printf("edata: %lx\n", (unsigned long) &edata);
    (void) k_printf("end: %lx\n", (unsigned long) &end);

    stop(init_free_physical_memory());

    stop(check_physical_memory());

    stop(!(pml4_pa = create_kernel_virtual_memory_space()));

    stop(report_physical_memory());

    stop(check_physical_memory());

    switch_pml4_pa(pml4_pa);

    (void) k_printf("Initialise process...\n");

    stop(start_init_process());
}
