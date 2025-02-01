;
; Copyright (c) 2024, 2025 Logan Ryan McLintock
;
; Permission to use, copy, modify, and/or distribute this software for any
; purpose with or without fee is hereby granted, provided that the above
; copyright notice and this permission notice appear in all copies.
;
; THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
; WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
; MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
; ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
; WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
; ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
; OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
;

; Shared definitions for Toucanix.

DISK equ 0x80
DISK_ADDRESS_PACKET_SIZE equ 16
EXTENDED_READ_FUNCTION_CODE equ 0x42
BIOS_DISK_SERVICES equ 0x13

MBR_SECTOR equ 1
NUM_OF_LOADER_SECTORS_TO_READ equ 1

BYTES_PER_DOUBLE_WORD equ 4
BYTES_PER_BLOCK equ 512

MBR_ADDRESS equ 0x7c00
VIDEO_ADDRESS equ 0xb8000
VIDEO_SEGMENT equ VIDEO_ADDRESS / 16
LOADER_ADDRESS equ MBR_ADDRESS + BYTES_PER_BLOCK

KERNEL_ORIGINAL_ADDRESS equ 0x10000
KERNEL_ORIGINAL_SEGMENT equ KERNEL_ORIGINAL_ADDRESS / 16
KERNEL_ORIGINAL_OFFSET equ KERNEL_ORIGINAL_ADDRESS % 16

NUM_OF_KERNEL_SECTORS_TO_READ equ 120
KERNEL_SIZE equ NUM_OF_KERNEL_SECTORS_TO_READ * BYTES_PER_BLOCK

KERNEL_ADDRESS equ 0x200000

; Colours.
RED equ 4

; Light.
GREY equ 7
GREEN equ 0xa

MAGENTA equ 5
YELLOW equ 0xe

YELLOW_ON_MAGENTA equ MAGENTA << 4 | YELLOW


USER_RING equ 3

PRESENT_BIT_SET                 equ 1 << 7
DESCRIPTOR_PRIVILEGE_LEVEL_USER equ USER_RING << 5
TYPE_IS_CODE_OR_DATA_SEGMENT    equ 1 << 4
EXECUTABLE                      equ 1 << 3
CODE_READ_OR_DATA_WRITE_ACCESS  equ 1 << 1


CODE_ACCESS_BYTE equ PRESENT_BIT_SET  \
    | TYPE_IS_CODE_OR_DATA_SEGMENT    \
    | EXECUTABLE

GRANULARITY_4_KIB   equ 1 << 3
SIZE_32_BIT_SEGMENT equ 1 << 2
LONG_MODE_CODE      equ 1 << 1

NULL_SEGMENT equ 0
CODE_SEGMENT_INDEX equ 1
CODE_SELECTOR equ CODE_SEGMENT_INDEX << 3
