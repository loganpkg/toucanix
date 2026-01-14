;>
;; Copyright (c) 2024, 2025 Logan Ryan McLintock
;;
;; Permission to use, copy, modify, and/or distribute this software for any
;; purpose with or without fee is hereby granted, provided that the above
;; copyright notice and this permission notice appear in all copies.
;;
;; THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
;; WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
;; MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
;; ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
;; WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
;; ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
;; OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
;<

;>
;; Shared definitions for Toucanix.
;; The C header file is generated from the assembly include file,
;; so do not edit the C header file directly.
;<

;+ #ifndef DEFS_H
;+ #define DEFS_H

;+ extern int dummy;

; Disk image size.
CYLINDERS equ 20
HEADS     equ 16
SECTORS   equ 63


; Disk image sectors.
MBR_SECTOR     equ   1
PRINT_SECTORS  equ   1
LOADER_SECTORS equ   2
KERNEL_SECTORS equ 120
USER_A_SECTORS equ   6
USER_B_SECTORS equ   6


BYTES_PER_SECTOR equ 512


; Disk starting sectors.
PRINT_START_SECTOR  equ MBR_SECTOR
LOADER_START_SECTOR equ PRINT_START_SECTOR  + PRINT_SECTORS
KERNEL_START_SECTOR equ LOADER_START_SECTOR + LOADER_SECTORS
USER_A_START_SECTOR equ KERNEL_START_SECTOR + KERNEL_SECTORS
USER_B_START_SECTOR equ USER_A_START_SECTOR + USER_A_SECTORS

KERNEL_SIZE equ KERNEL_SECTORS * BYTES_PER_SECTOR
USER_A_SIZE equ USER_A_SECTORS * BYTES_PER_SECTOR
USER_B_SIZE equ USER_B_SECTORS * BYTES_PER_SECTOR


DWORD_SIZE       equ   4
PAGE_TABLE_SIZE equ 0x1000
BYTES_PER_PAGE_TABLE_ENTRY equ 8
NUM_GIB_MAPPED equ PAGE_TABLE_SIZE / BYTES_PER_PAGE_TABLE_ENTRY

MAX_MAPPED_VA_EXCL equ KERNEL_SPACE_VA + (NUM_GIB_MAPPED << EXP_1_GIB)




; Physcial addresses.
MBR_PA                    equ   0x7c00
PRINT_PA                  equ MBR_PA + MBR_SECTOR * BYTES_PER_SECTOR
    ROW_PA                equ PRINT_PA
    COL_PA                equ ROW_PA + 4
    PRINT_FUNC            equ COL_PA + 4
LOADER_PA                 equ PRINT_PA + PRINT_SECTORS * BYTES_PER_SECTOR
MEMORY_MAP_ENTRY_COUNT_PA equ   0x9000
    MEMORY_MAP_PA         equ MEMORY_MAP_ENTRY_COUNT_PA + DWORD_SIZE
KERNEL_ORIGINAL_PA        equ  0x10000
USER_A_PA                 equ  0x20000
USER_B_PA                 equ  0x30000
PML4_PA                   equ  0x70000
    PDPT_PA               equ PML4_PA + PAGE_TABLE_SIZE
VIDEO_PA                  equ  0xb8000
KERNEL_PA                 equ 0x200000




; Segments and offsets.
VIDEO_SEGMENT equ VIDEO_PA / 16

KERNEL_ORIGINAL_SEGMENT equ KERNEL_ORIGINAL_PA / 16
KERNEL_ORIGINAL_OFFSET  equ KERNEL_ORIGINAL_PA % 16

USER_A_SEGMENT equ USER_A_PA / 16
USER_A_OFFSET  equ USER_A_PA % 16

USER_B_SEGMENT equ USER_B_PA / 16
USER_B_OFFSET  equ USER_B_PA % 16



; Virtual addresses.
USER_EXEC_START_VA        equ           0x400000
NON_CANONICAL_MIN_VA      equ 0x0000800000000000
; push decrements the stack before storing.
USER_STACK_VA equ NON_CANONICAL_MIN_VA
; Start of the higher 48-bit canonical address region.
KERNEL_SPACE_VA           equ 0xffff800000000000
PRINT_VA                  equ KERNEL_SPACE_VA + PRINT_PA
MEMORY_MAP_ENTRY_COUNT_VA equ KERNEL_SPACE_VA + MEMORY_MAP_ENTRY_COUNT_PA
    MEMORY_MAP_VA         equ KERNEL_SPACE_VA + MEMORY_MAP_PA
PML4_VA                   equ KERNEL_SPACE_VA + PML4_PA
VIDEO_VA                  equ KERNEL_SPACE_VA + VIDEO_PA
KERNEL_VA                 equ KERNEL_SPACE_VA + KERNEL_PA
    KERNEL_STACK_VA       equ KERNEL_VA




EXP_2_MIB equ 21
EXP_1_GIB equ 30

PAGE_SIZE equ 1 << EXP_2_MIB


PAGE_PRESENT    equ 1
READ_AND_WRITE  equ 1 << 1
USER_ACCESS     equ 1 << 2
; Page Size attribute.
PS              equ 1 << 7


; Newline character.
NL equ 10


DISK                        equ 0x80
DISK_PA_PACKET_SIZE         equ   16
EXTENDED_READ_FUNCTION_CODE equ 0x42
BIOS_DISK_SERVICES          equ 0x13



PIC_MASTER_COMMAND equ 0x20



SCREEN_WIDTH equ 80
SCREEN_HEIGHT equ 25

BYTES_PER_SCREEN_CHAR equ 2
BYTES_PER_LINE equ SCREEN_WIDTH * BYTES_PER_SCREEN_CHAR


; Colours.
BLUE    equ   1
RED     equ   4
; Light.
GREY    equ   7
GREEN   equ 0xa
MAGENTA equ   5
YELLOW  equ 0xe

YELLOW_ON_MAGENTA equ MAGENTA << 4 | YELLOW

DEFAULT_COLOUR equ GREEN

USER_RING equ 3

PRESENT_BIT_SET                 equ         1 << 7
DESCRIPTOR_PRIVILEGE_LEVEL_USER equ USER_RING << 5
CODE_OR_DATA_SEGMENT_TYPE       equ         1 << 4
EXEC                            equ         1 << 3
CODE_READ_OR_DATA_WRITE_ACCESS  equ         1 << 1


CODE_ACCESS_BYTE equ PRESENT_BIT_SET | CODE_OR_DATA_SEGMENT_TYPE | EXEC

GRANULARITY_4_KIB   equ 1 << 3
SIZE_32_BIT_SEGMENT equ 1 << 2
LONG_MODE_CODE      equ 1 << 1

NULL_SEGMENT equ 0
CODE_SEGMENT_INDEX equ 1
CODE_SELECTOR equ CODE_SEGMENT_INDEX << 3


; Must be <= INT_MAX.
BUF_SIZE equ 1024


; Unsigned integer limits.
U64_MAX_DEC_DIGITS equ 20
U64_MAX_HEX_DIGITS equ 18
U64_MAX equ 0xFFFFFFFFFFFFFFFF
U32_MAX equ         0xFFFFFFFF


; Standard file descriptors.
STDIN_FILENO  equ 0
STDOUT_FILENO equ 1
STDERR_FILENO equ 2

SYS_ERROR    equ   -1
SOFTWARE_INT equ 0x80

; Software system call numbers.
SYS_CALL_WRITE equ 0
SYS_CALL_SLEEP equ 1


; Timer.
EVENTS_PER_SECOND equ 100


; Wait reasons.
TIMER_WAIT equ 0


; GDT.
USER_CODE_SEGMENT_INDEX equ 2
USER_DATA_SEGMENT_INDEX equ 3

USER_CODE_SELECTOR equ USER_CODE_SEGMENT_INDEX << 3 | USER_RING
USER_DATA_SELECTOR equ USER_DATA_SEGMENT_INDEX << 3 | USER_RING


TSS_SIZE equ 104


;+ #endif
