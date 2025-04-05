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


#ifndef MEMORY_H
#define MEMORY_H

#include "stdint.h"


/* From memory.asm file. */
void switch_pml4_pa(uint64_t new_pml4_start_pa);

/* From memory.c file. */
int print_memory_map_pa(void);
void free_page_pa(uint64_t start_page_pa);
uint64_t allocate_page_pa(void);
int init_free_physical_memory(void);
int report_physical_memory(void);
int check_physical_memory(void);
void free_4_level_paging(uint64_t pml4_pa);
uint64_t create_kernel_virtual_memory_space(void);
uint64_t create_user_virtual_memory_space(uint64_t exec_start_va,
                                          uint64_t exec_size);



#endif
