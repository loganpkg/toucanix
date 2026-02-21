/*
 * Copyright (c) 2024-2026 Logan Ryan McLintock. All rights reserved.
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
 * Shared definitions for Toucanix.
 * The C header file is generated from the assembly include file,
 * so do not edit the C header file directly.
 */

#ifndef DEFS_H
#define DEFS_H

extern int dummy;

/* Disk image size. */
#define CYLINDERS 20
#define HEADS     16
#define SECTORS   63

/* Disk image sectors. */
#define MBR_SECTOR     1
#define PRINT_SECTORS  1
#define LOADER_SECTORS 2
#define KERNEL_SECTORS 120
#define USER_A_SECTORS 6
#define USER_B_SECTORS 6

#define BYTES_PER_SECTOR 512

/* Disk starting sectors. */
#define PRINT_START_SECTOR  MBR_SECTOR
#define LOADER_START_SECTOR (PRINT_START_SECTOR + PRINT_SECTORS)
#define KERNEL_START_SECTOR (LOADER_START_SECTOR + LOADER_SECTORS)
#define USER_A_START_SECTOR (KERNEL_START_SECTOR + KERNEL_SECTORS)
#define USER_B_START_SECTOR (USER_A_START_SECTOR + USER_A_SECTORS)

#define KERNEL_SIZE (KERNEL_SECTORS * BYTES_PER_SECTOR)
#define USER_A_SIZE (USER_A_SECTORS * BYTES_PER_SECTOR)
#define USER_B_SIZE (USER_B_SECTORS * BYTES_PER_SECTOR)

#define DWORD_SIZE                 4
#define PAGE_TABLE_SIZE            0x1000
#define BYTES_PER_PAGE_TABLE_ENTRY 8
#define NUM_GIB_MAPPED             (PAGE_TABLE_SIZE / BYTES_PER_PAGE_TABLE_ENTRY)

#define MAX_MAPPED_VA_EXCL                                                    \
    (KERNEL_SPACE_VA + ((uint64_t) NUM_GIB_MAPPED << EXP_1_GIB))

/* Physcial addresses. */
#define MBR_PA                    0x7c00
#define PRINT_PA                  (MBR_PA + MBR_SECTOR * BYTES_PER_SECTOR)
#define ROW_PA                    PRINT_PA
#define COL_PA                    (ROW_PA + 4)
#define PRINT_FUNC                (COL_PA + 4)
#define LOADER_PA                 (PRINT_PA + PRINT_SECTORS * BYTES_PER_SECTOR)
#define MEMORY_MAP_ENTRY_COUNT_PA 0x9000
#define MEMORY_MAP_PA             (MEMORY_MAP_ENTRY_COUNT_PA + DWORD_SIZE)
#define KERNEL_ORIGINAL_PA        0x10000
#define USER_A_PA                 0x20000
#define USER_B_PA                 0x30000
#define PML4_PA                   0x70000
#define PDPT_PA                   (PML4_PA + PAGE_TABLE_SIZE)
#define VIDEO_PA                  0xb8000
#define KERNEL_PA                 0x200000

/* Segments and offsets. */
#define VIDEO_SEGMENT (VIDEO_PA / 16)

#define KERNEL_ORIGINAL_SEGMENT (KERNEL_ORIGINAL_PA / 16)
#define KERNEL_ORIGINAL_OFFSET  (KERNEL_ORIGINAL_PA % 16)

#define USER_A_SEGMENT (USER_A_PA / 16)
#define USER_A_OFFSET  (USER_A_PA % 16)

#define USER_B_SEGMENT (USER_B_PA / 16)
#define USER_B_OFFSET  (USER_B_PA % 16)

/* Virtual addresses. */
#define USER_EXEC_START_VA   0x400000
#define NON_CANONICAL_MIN_VA 0x0000800000000000
/* push decrements the stack before storing. */
#define USER_STACK_VA NON_CANONICAL_MIN_VA
/* Start of the higher 48-bit canonical address region. */
#define KERNEL_SPACE_VA           0xffff800000000000
#define PRINT_VA                  (KERNEL_SPACE_VA + PRINT_PA)
#define MEMORY_MAP_ENTRY_COUNT_VA (KERNEL_SPACE_VA + MEMORY_MAP_ENTRY_COUNT_PA)
#define MEMORY_MAP_VA             (KERNEL_SPACE_VA + MEMORY_MAP_PA)
#define PML4_VA                   (KERNEL_SPACE_VA + PML4_PA)
#define VIDEO_VA                  (KERNEL_SPACE_VA + VIDEO_PA)
#define KERNEL_VA                 (KERNEL_SPACE_VA + KERNEL_PA)
#define KERNEL_STACK_VA           KERNEL_VA

#define EXP_2_MIB 21
#define EXP_1_GIB 30

#define PAGE_SIZE (1 << EXP_2_MIB)

#define PAGE_PRESENT   1
#define READ_AND_WRITE (1 << 1)
#define USER_ACCESS    (1 << 2)
/* Page Size attribute. */
#define PS (1 << 7)

/* Newline character. */
#define NL 10

#define DISK                        0x80
#define DISK_PA_PACKET_SIZE         16
#define EXTENDED_READ_FUNCTION_CODE 0x42
#define BIOS_DISK_SERVICES          0x13

#define PIC_MASTER_COMMAND 0x20

#define SCREEN_WIDTH  80
#define SCREEN_HEIGHT 25

#define BYTES_PER_SCREEN_CHAR 2
#define BYTES_PER_LINE        (SCREEN_WIDTH * BYTES_PER_SCREEN_CHAR)

/* Colours. */
#define BLUE 1
#define RED  4
/* Light. */
#define GREY    7
#define GREEN   0xa
#define MAGENTA 5
#define YELLOW  0xe

#define YELLOW_ON_MAGENTA (MAGENTA << 4 | YELLOW)

#define DEFAULT_COLOUR GREEN

#define USER_RING 3

#define PRESENT_BIT_SET                 (1 << 7)
#define DESCRIPTOR_PRIVILEGE_LEVEL_USER (USER_RING << 5)
#define CODE_OR_DATA_SEGMENT_TYPE       (1 << 4)
#define EXEC                            (1 << 3)
#define CODE_READ_OR_DATA_WRITE_ACCESS  (1 << 1)

#define CODE_ACCESS_BYTE (PRESENT_BIT_SET | CODE_OR_DATA_SEGMENT_TYPE | EXEC)

#define GRANULARITY_4_KIB   (1 << 3)
#define SIZE_32_BIT_SEGMENT (1 << 2)
#define LONG_MODE_CODE      (1 << 1)

#define NULL_SEGMENT       0
#define CODE_SEGMENT_INDEX 1
#define CODE_SELECTOR      (CODE_SEGMENT_INDEX << 3)

/* Must be <= INT_MAX. */
#define BUF_SIZE 1024

/* Unsigned integer limits. */
#define U64_MAX_DEC_DIGITS 20
#define U64_MAX_HEX_DIGITS 18
#define U64_MAX            0xFFFFFFFFFFFFFFFF
#define U32_MAX            0xFFFFFFFF

/* Standard file descriptors. */
#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#define SYS_ERROR    (-1)
#define SOFTWARE_INT 0x80

/* Software system call numbers. */
#define SYS_CALL_WRITE 0
#define SYS_CALL_SLEEP 1

/* Timer. */
#define EVENTS_PER_SECOND 100

/* Wait reasons. */
#define TIMER_WAIT 0

/* GDT. */
#define USER_CODE_SEGMENT_INDEX 2
#define USER_DATA_SEGMENT_INDEX 3

#define USER_CODE_SELECTOR (USER_CODE_SEGMENT_INDEX << 3 | USER_RING)
#define USER_DATA_SELECTOR (USER_DATA_SEGMENT_INDEX << 3 | USER_RING)

#define TSS_SIZE 104

#endif
