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
