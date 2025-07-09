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


#ifndef ADDRESS_H
#define ADDRESS_H


#define EXP_2_MIB 21
#define PAGE_SIZE (1 << EXP_2_MIB)

/*
 * Aligns up to the next page if not already aligned. The minus one is so
 * that an already aligned address will not move up to the next page.
 */
#define align_to_page(a) (((a) + PAGE_SIZE - 1) >> EXP_2_MIB << EXP_2_MIB)

/* Truncates an address down to the start of its page. */
#define truncate_to_page(a) ((a) >> EXP_2_MIB << EXP_2_MIB)


/* Converts a physical address to a virtual address. */
#define pa_to_va(a) ((a) + KERNEL_SPACE_VA)

/* Converts a virtual address to a physical address. */
#define va_to_pa(a) ((a) - KERNEL_SPACE_VA)


#define KERNEL_PA 0x200000
#define KERNEL_SPACE_VA 0xffff800000000000

#define VIDEO_PA 0xb8000
#define VIDEO_VA pa_to_va(VIDEO_PA)

#define PML4_PA 0x70000
#define PML4_VA pa_to_va(PML4_PA)

#define USER_EXEC_START_VA 0x400000

#define USER_SPACE_TOP_VA ((uint64_t) 0x7fffffffffff)
#define USER_STACK_PAGE_VA truncate_to_page(USER_SPACE_TOP_VA)

#endif
