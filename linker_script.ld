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


KERNEL_PA = 0x200000;
KERNEL_SPACE_VA = 0xffff800000000000;
KERNEL_VA = KERNEL_SPACE_VA + KERNEL_PA;
ALIGN_BYTES = 16;

ENTRY(_start)
OUTPUT_FORMAT(elf64-x86-64)

SECTIONS {
    /* Dot . is the output location counter. */
    . = KERNEL_VA;

    .text : { *(.text .text.*) }
    PROVIDE(etext = .);

    .rodata : { *(.rodata .rodata.*) }

     . = ALIGN(ALIGN_BYTES);
    .data : { *(.data .data.*) }
    PROVIDE(edata = .);

    .bss : { *(.bss .bss.*) }
    PROVIDE(end = .);
}
