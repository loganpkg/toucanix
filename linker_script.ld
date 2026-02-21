/*
 * Copyright (c) 2025, 2026 Logan Ryan McLintock. All rights reserved.
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

/*
 * Linker script. The user version is generated from the kernel version,
 * so do not directly edit the user version.
 */

KERNEL_PA = 0x200000; /*-*/
KERNEL_SPACE_VA = 0xffff800000000000; /*-*/
KERNEL_VA = KERNEL_SPACE_VA + KERNEL_PA; /*-*/
/*+ USER_EXEC_START_VA = 0x400000; +*/
ALIGN_BYTES = 16;

ENTRY(_start)
OUTPUT_FORMAT(elf64-x86-64)

SECTIONS {
    /* Dot . is the output location counter. */
    . = KERNEL_VA; /*-*/
    /*+ . = USER_EXEC_START_VA; +*/

    .text : { *(.text .text.*) }
    PROVIDE(etext = .);

    .rodata : { *(.rodata .rodata.* .lrodata.str1.1) }

     . = ALIGN(ALIGN_BYTES);
    .data : { *(.data .data.*) }
    PROVIDE(edata = .);

    .bss : { *(.bss .bss.*) }
    PROVIDE(end = .);
}
