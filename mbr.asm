;
; Copyright (c) 2024 Logan Ryan McLintock
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

%include "defs.asm"


%define BIOS_VIDEO_SERVICES 0x10
%define SET_VIDEO_MODE 0
%define TEXT_MODE_80_x_25_COLOUR 3


%define BYTES_PER_BLOCK 512
%define NUM_OF_PARTITION_ENTRIES 4
%define NUM_OF_EMPTY_PARTITION_ENTRIES 3
%define BYTES_PER_PARTITION_ENTRY 16
%define BOOT_SIGNATURE 0x55, 0xAA
%define BOOT_SIGNATURE_LEN 2
bytes_before_partition_table equ (BYTES_PER_BLOCK \
    - BYTES_PER_PARTITION_ENTRY * NUM_OF_PARTITION_ENTRIES - BOOT_SIGNATURE_LEN)

%define BOOTABLE 0x80
%define FIRST_CYLINDER_IN_PARTITION 0
%define FIRST_HEAD_IN_PARTITION 0
%define FIRST_SECTOR_IN_PARTITION 2
%define CYLINDERS 20
%define HEADS 16
%define SECTORS 63

last_cylinder equ CYLINDERS - 1
upper_two_bits_of_last_cylinder equ (last_cylinder >> 2) & 1100_0000b
lower_eight_bits_of_last_cylinder equ last_cylinder & 1111_1111b

last_head equ HEADS - 1

; Sector index commences at 1, not 0.
last_sector equ SECTORS


%define PA_RISC_LINUX_PARTITION_TYPE 0xf0
%define LBA_FIRST_SECTOR_IN_PARTITION 1
%define MBR_SECTOR 1
number_sectors_in_partition equ (CYLINDERS * HEADS * SECTORS - MBR_SECTOR)

blank_partition_entries_size equ \
    BYTES_PER_PARTITION_ENTRY * NUM_OF_EMPTY_PARTITION_ENTRIES

%define EXTENDED_READ_FUNCTION_CODE 0x42
%define DISK_ADDRESS_PACKET_SIZE 16
%define NUM_SECTORS_TO_READ 5
%define LOADER_START_SECTOR 1

%define DISK 0x80

%define BIOS_DISK_SERVICES 0x13


; In Real mode: Intel 8086.
[BITS 16]
[ORG MBR_ADDRESS]

xor ax, ax

; Zero segment registers.
mov ds, ax
mov es, ax
mov ss, ax

; Set stack point (grows downwards).
mov sp, MBR_ADDRESS


mov ah, SET_VIDEO_MODE
mov al, TEXT_MODE_80_x_25_COLOUR
int BIOS_VIDEO_SERVICES


; Load the loader into memory.
mov dl, DISK
xor ax, ax
mov ds, ax
mov si, loader_disk_address_packet
mov ah, EXTENDED_READ_FUNCTION_CODE
int BIOS_DISK_SERVICES
jc error

jmp loader_address


error:
mov ax, video_segment
mov es, ax
xor di, di
mov byte [es:di], 'E'
mov byte [es:di + 1], yellow_on_magenta

done:
hlt
jmp done


; Data.


; For reading loader into memory.
loader_disk_address_packet:
db DISK_ADDRESS_PACKET_SIZE
db 0
dw NUM_SECTORS_TO_READ
dd loader_address
dq LOADER_START_SECTOR




; Zero to the start of the partition table.
times bytes_before_partition_table - ($ - $$) db 0


;;;;;;;;;;;;;;;;;;;;
; Partition entry. ;
;;;;;;;;;;;;;;;;;;;;

db BOOTABLE

db FIRST_HEAD_IN_PARTITION
db FIRST_SECTOR_IN_PARTITION
db FIRST_CYLINDER_IN_PARTITION

db PA_RISC_LINUX_PARTITION_TYPE

db last_head
db upper_two_bits_of_last_cylinder | last_sector
db lower_eight_bits_of_last_cylinder

dd LBA_FIRST_SECTOR_IN_PARTITION

dd number_sectors_in_partition

; Add blank entries.
times blank_partition_entries_size db 0

db BOOT_SIGNATURE
