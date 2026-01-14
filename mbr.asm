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
