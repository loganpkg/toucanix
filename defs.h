/*
 * Copyright (c) 2024, 2025 Logan Ryan McLintock
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

/*
 * Shared definitions for Toucanix.
 * The C header file is generated from the assembly include file,
 * so do not edit the C header file directly.
 */

#ifndef DEFS_H
#define DEFS_H


/* Newline character. */
#define NL 10

#define DISK 0x80
#define DISK_PA_PACKET_SIZE 16
#define EXTENDED_READ_FUNCTION_CODE 0x42
#define BIOS_DISK_SERVICES 0x13

#define MBR_SECTOR 1
#define PRINT_SECTORS 1
#define LOADER_SECTORS 2
#define KERNEL_SECTORS 120
#define USER_SECTORS 12

#define PRINT_START_SECTOR MBR_SECTOR
#define LOADER_START_SECTOR (PRINT_START_SECTOR + PRINT_SECTORS)
#define KERNEL_START_SECTOR (LOADER_START_SECTOR + LOADER_SECTORS)
#define USER_START_SECTOR (KERNEL_START_SECTOR + KERNEL_SECTORS)

#define BYTES_PER_DOUBLE_WORD 4
#define BYTES_PER_SECTOR 512

#define MBR_PA 0x7c00
#define PRINT_PA (MBR_PA + (MBR_SECTOR * BYTES_PER_SECTOR))
#define LOADER_PA (PRINT_PA + (PRINT_SECTORS * BYTES_PER_SECTOR))

#define PIC_MASTER_COMMAND  0x20

#define VIDEO_PA 0xb8000
#define VIDEO_SEGMENT (VIDEO_PA / 16)

#define ROW_PA PRINT_PA
#define COL_PA (PRINT_PA + 4)
#define PRINT_FUNC (COL_PA + 4)


#define KERNEL_ORIGINAL_PA 0x10000
#define KERNEL_ORIGINAL_SEGMENT (KERNEL_ORIGINAL_PA / 16)
#define KERNEL_ORIGINAL_OFFSET (KERNEL_ORIGINAL_PA % 16)

#define USER_ORIGINAL_PA 0x20000
#define USER_ORIGINAL_SEGMENT (USER_ORIGINAL_PA / 16)
#define USER_ORIGINAL_OFFSET (USER_ORIGINAL_PA % 16)

#define KERNEL_SIZE (KERNEL_SECTORS * BYTES_PER_SECTOR)
#define USER_SIZE (USER_SECTORS * BYTES_PER_SECTOR)

#define KERNEL_PA 0x200000

/* Start of the higher 48-bit canonical address region. */
#define KERNEL_SPACE_VA 0xffff800000000000
#define KERNEL_VA (KERNEL_SPACE_VA + KERNEL_PA)


#define PML4_PA 0x70000
#define PML4E_IDENTITY PML4_PA

#define PML4E_IDENTITY_VA (KERNEL_SPACE_VA + PML4E_IDENTITY)


/* Colours. */
#define BLUE 1
#define RED 4

/* Light. */
#define GREY 7
#define GREEN 0xa

#define MAGENTA 5
#define YELLOW 0xe

#define YELLOW_ON_MAGENTA (MAGENTA << 4 | YELLOW)

#define DEFAULT_COLOUR GREEN

#define USER_RING 3

#define PRESENT_BIT_SET                 (1 << 7)
#define DESCRIPTOR_PRIVILEGE_LEVEL_USER (USER_RING << 5)
#define TYPE_IS_CODE_OR_DATA_SEGMENT    (1 << 4)
#define EXEC                            (1 << 3)
#define CODE_READ_OR_DATA_WRITE_ACCESS  (1 << 1)


#define CODE_ACCESS_BYTE (PRESENT_BIT_SET \
    | TYPE_IS_CODE_OR_DATA_SEGMENT    \
    | EXEC)

#define GRANULARITY_4_KIB   (1 << 3)
#define SIZE_32_BIT_SEGMENT (1 << 2)
#define LONG_MODE_CODE      (1 << 1)

#define NULL_SEGMENT 0
#define CODE_SEGMENT_INDEX 1
#define CODE_SELECTOR (CODE_SEGMENT_INDEX << 3)


/* Must be <= INT_MAX. */
#define BUF_SIZE 1024

#define U64_MAX_DIGITS 20
#define U64_MAX_HEX 18

/* Standard file descriptors. */
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#define SYS_ERROR -1
#define SOFTWARE_INT 0x80

/* Software system call numbers. */
#define SYS_CALL_WRITE 0


#endif
