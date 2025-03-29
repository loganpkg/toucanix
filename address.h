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


/* Converts a physical address to a virtual address. */
#define pa_to_va(a) ((a) + KERNEL_SPACE_VA)

/* Converts a virtual address to a physical address. */
#define va_to_pa(a) ((a) - KERNEL_SPACE_VA)


#define KERNEL_PA 0x200000
#define KERNEL_SPACE_VA 0xffff800000000000
#define KERNEL_VA pa_to_va(KERNEL_PA)

#define VIDEO_PA 0xb8000
#define VIDEO_VA pa_to_va(VIDEO_PA)

#define PML4_PA 0x70000
#define PML4_VA pa_to_va(PML4_PA)

#define USER_EXEC_START_VA 0x400000

#endif
