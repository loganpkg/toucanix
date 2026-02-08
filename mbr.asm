;
; Copyright (c) 2024-2026 Logan Ryan McLintock. All rights reserved.
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions
; are met:
; 1. Redistributions of source code must retain the above copyright
;    notice, this list of conditions and the following disclaimer.
; 2. Redistributions in binary form must reproduce the above copyright
;    notice, this list of conditions and the following disclaimer in the
;    documentation and/or other materials provided with the distribution.
;
; THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
; ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
; IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
; ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
; FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
; DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
; OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
; HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
; LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
; OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
; SUCH DAMAGE.
;

; Master boot record for Toucanix.

%include "defs.inc"


BIOS_VIDEO_SERVICES equ 0x10
SET_VIDEO_MODE equ 0
TEXT_MODE_80_x_25_COLOUR equ 3

SET_CURSOR_SHAPE equ 1
INVISIBLE_CURSOR_BLINK equ 1 << 5
BOTTOM_SCAN_LINE equ 15


NUM_OF_PARTITION_ENTRIES equ 4
NUM_OF_EMPTY_PARTITION_ENTRIES equ 3
BYTES_PER_PARTITION_ENTRY equ 16
BOOT_SIGNATURE equ 0xAA55
BOOT_SIGNATURE_LEN equ 2
BYTES_BEFORE_PARTITION_TABLE equ (BYTES_PER_SECTOR \
    - BYTES_PER_PARTITION_ENTRY * NUM_OF_PARTITION_ENTRIES \
    - BOOT_SIGNATURE_LEN)

BOOTABLE equ 0x80
FIRST_CYLINDER_IN_PARTITION equ 0
FIRST_HEAD_IN_PARTITION equ 0
FIRST_SECTOR_IN_PARTITION equ 2

LAST_CYLINDER equ CYLINDERS - 1
LAST_HEAD equ HEADS - 1

; Sector index commences at 1, not 0.
LAST_SECTOR equ SECTORS

BLANK_PARTITION_ENTRIES_SIZE equ \
    BYTES_PER_PARTITION_ENTRY * NUM_OF_EMPTY_PARTITION_ENTRIES

PA_RISC_LINUX_PARTITION_TYPE equ 0xf0
LBA_FIRST_SECTOR_IN_PARTITION equ 1
NUMBER_SECTORS_IN_PARTITION equ CYLINDERS * HEADS * SECTORS - MBR_SECTOR



; In Real mode: Intel 8086.
[BITS 16]
[ORG MBR_PA]

xor ax, ax

; Zero segment registers.
mov ds, ax
mov es, ax
mov ss, ax

; Set stack point (grows downwards).
mov sp, MBR_PA


mov ah, SET_VIDEO_MODE
mov al, TEXT_MODE_80_x_25_COLOUR
int BIOS_VIDEO_SERVICES


mov ah, SET_CURSOR_SHAPE
mov ch, INVISIBLE_CURSOR_BLINK
mov cl, BOTTOM_SCAN_LINE
int BIOS_VIDEO_SERVICES


; Load print into memory.
mov dl, DISK
xor ax, ax
mov ds, ax
mov si, print_disk_address_packet
mov ah, EXTENDED_READ_FUNCTION_CODE
int BIOS_DISK_SERVICES
jc error_p


xor ax, ax
mov ds, ax
mov si, welcome_str
mov bl, DEFAULT_COLOUR
call PRINT_FUNC


; Load the loader into memory.
mov dl, DISK
xor ax, ax
mov ds, ax
mov si, loader_disk_address_packet
mov ah, EXTENDED_READ_FUNCTION_CODE
int BIOS_DISK_SERVICES
jc error_l

jmp LOADER_PA


error_p:
mov ax, VIDEO_SEGMENT
mov es, ax
xor di, di
mov byte [es:di], 'P'
mov byte [es:di + 1], YELLOW_ON_MAGENTA
.done:
hlt
jmp .done


error_l:
xor ax, ax
mov ds, ax
mov si, loader_failed_str
mov bl, YELLOW_ON_MAGENTA
call PRINT_FUNC
.done:
hlt
jmp .done


; Data.

welcome_str: db 'Welcome to Toucanix', NL, 0
loader_failed_str: db 'ERROR: Failed to load the loader', NL, 0


; For reading print into memory.
print_disk_address_packet:
db DISK_PA_PACKET_SIZE
db 0
dw PRINT_SECTORS
dd PRINT_PA
dq PRINT_START_SECTOR



; For reading loader into memory.
loader_disk_address_packet:
db DISK_PA_PACKET_SIZE
db 0
dw LOADER_SECTORS
dd LOADER_PA
dq LOADER_START_SECTOR




; Zero to the start of the partition table.
times BYTES_BEFORE_PARTITION_TABLE - ($ - $$) db 0


;;;;;;;;;;;;;;;;;;;;
; Partition entry. ;
;;;;;;;;;;;;;;;;;;;;

db BOOTABLE

db FIRST_HEAD_IN_PARTITION
db FIRST_SECTOR_IN_PARTITION
db FIRST_CYLINDER_IN_PARTITION

db PA_RISC_LINUX_PARTITION_TYPE


db LAST_HEAD
db LAST_CYLINDER >> 2 & 0xc0 | LAST_SECTOR
db LAST_CYLINDER & 0xff

dd LBA_FIRST_SECTOR_IN_PARTITION

dd NUMBER_SECTORS_IN_PARTITION

; Add blank entries.
times BLANK_PARTITION_ENTRIES_SIZE db 0

dw BOOT_SIGNATURE
