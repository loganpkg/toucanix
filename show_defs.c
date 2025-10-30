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

#include <stdio.h>
#include "defs.h"


#define print_macro(m) printf("%s: %#lx\n", #m, (unsigned long) m)
#define print_macro_dec(m) printf("%s: %d\n", #m, m)


int main(void)
{
    printf("Disk image sectors:\n");
    print_macro_dec(MBR_SECTOR);
    print_macro_dec(PRINT_SECTORS);
    print_macro_dec(LOADER_SECTORS);
    print_macro_dec(KERNEL_SECTORS);
    print_macro_dec(USER_A_SECTORS);
    print_macro_dec(USER_B_SECTORS);


    printf("Disk starting sectors:\n");
    print_macro_dec(PRINT_START_SECTOR);
    print_macro_dec(LOADER_START_SECTOR);
    print_macro_dec(KERNEL_START_SECTOR);
    print_macro_dec(USER_A_START_SECTOR);
    print_macro_dec(USER_B_START_SECTOR);

    printf("Identity mapping:\n");
    print_macro_dec(NUM_GIB_MAPPED);

    printf("Physical memory addresses:\n");
    print_macro(MBR_PA);
    print_macro(PRINT_PA);
    print_macro(ROW_PA);
    print_macro(COL_PA);
    print_macro(PRINT_FUNC);
    print_macro(LOADER_PA);
    print_macro(MEMORY_MAP_ENTRY_COUNT_PA);
    print_macro(MEMORY_MAP_PA);
    print_macro(KERNEL_ORIGINAL_PA);
    print_macro(USER_A_PA);
    print_macro(USER_B_PA);
    print_macro(PML4_PA);
    print_macro(PDPT_PA);
    print_macro(VIDEO_PA);
    print_macro(KERNEL_PA);


    printf("Virtual memory addresses:\n");
    print_macro(USER_EXEC_START_VA);
    print_macro(NON_CANONICAL_MIN_VA);
    print_macro(USER_STACK_VA);
    print_macro(KERNEL_SPACE_VA);
    print_macro(PRINT_VA);
    print_macro(PML4_VA);
    print_macro(VIDEO_VA);
    print_macro(KERNEL_VA);
    print_macro(KERNEL_STACK_VA);


    return 0;
}
