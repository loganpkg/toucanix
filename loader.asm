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

; Loader for Toucanix.

%include "defs.asm"

%define SYSTEM_MEMORY_MAP_ADDRESS 0x9000
%define BIOS_SYSTEM_SERVICES 0x15
%define SYSTEM_ADDRESS_MAP_FUNCTION_CODE 0xe820
; SMAP. PAMS in little-endian.
%define SYSTEM_MAP_SIGNATURE 0x534D4150
%define ADDRESS_RANGE_DESCRIPTOR_SIZE 20
%define NUM_OF_KERNEL_SECTORS_TO_READ 120
kernel_start_sector equ MBR_SECTOR + NUM_OF_LOADER_SECTORS_TO_READ
%define NUM_OF_KERNEL_SECTORS_TO_READ 120


%define PRESENT_BIT_SET                   (1 << 7)
%define DESCRIPTOR_PRIVILEGE_LEVEL_KERNEL (0 << 5)
%define DESCRIPTOR_PRIVILEGE_LEVEL_USER   (3 << 5)
%define TYPE_IS_CODE_OR_DATA_SEGMENT      (1 << 4)
%define EXECUTABLE_FOR_CODE               (1 << 3)
%define NON_EXECUTABLE_FOR_DATA           (0 << 3)
%define CODE_RING_IN_DPL_ONLY             (0 << 2)
%define DATA_DIRECTION_GROWS_UP           (0 << 2)
%define CODE_READ_ACCESS                  (1 << 1)
%define DATA_WRITE_ACCESS                 (1 << 1)
%define ACCESSED_BIT_CLEAR                (0 << 0)

code_access_byte equ PRESENT_BIT_SET    \
    | DESCRIPTOR_PRIVILEGE_LEVEL_KERNEL \
    | TYPE_IS_CODE_OR_DATA_SEGMENT      \
    | EXECUTABLE_FOR_CODE               \
    | CODE_RING_IN_DPL_ONLY             \
    | CODE_READ_ACCESS                  \
    | ACCESSED_BIT_CLEAR

data_access_byte equ PRESENT_BIT_SET    \
    | DESCRIPTOR_PRIVILEGE_LEVEL_KERNEL \
    | TYPE_IS_CODE_OR_DATA_SEGMENT      \
    | NON_EXECUTABLE_FOR_DATA           \
    | DATA_DIRECTION_GROWS_UP           \
    | DATA_WRITE_ACCESS                 \
    | ACCESSED_BIT_CLEAR

%define GRANULARITY_4_KIB   (1 << 3)
%define SIZE_32_BIT_SEGMENT (1 << 2)
%define NOT_LONG_MODE_CODE  (0 << 1)
%define RESERVED_FLAG       (0 << 0)

flags_nibble equ GRANULARITY_4_KIB \
    | SIZE_32_BIT_SEGMENT          \
    | NOT_LONG_MODE_CODE           \
    | RESERVED_FLAG

%define BASE_ADDRESS 0
; %define BASE_ADDRESS 0xefcdab89

%define SEGMENT_LIMIT 0xfffff


%define NULL_SEGMENT 0
%define CODE_SEGMENT_INDEX 1
%define DATA_SEGMENT_INDEX 2

%define TABLE_INDICATOR_GDT (0 << 2)
%define REQUESTOR_PRIVILEGE_LEVEL_KERNEL 00b

code_selector equ CODE_SEGMENT_INDEX << 3 \
    | TABLE_INDICATOR_GDT                 \
    | REQUESTOR_PRIVILEGE_LEVEL_KERNEL

data_selector equ DATA_SEGMENT_INDEX << 3 \
    | TABLE_INDICATOR_GDT                 \
    | REQUESTOR_PRIVILEGE_LEVEL_KERNEL


%define IDT_INVALID_SIZE_MINUS_1 0
%define INTERRUPT_DESCRIPTOR_TABLE_INVALID_ADDRESS 0

%define PROTECTED_MODE 1


[BITS 16]
[ORG loader_address]


; Save the system memory map.
mov edi, SYSTEM_MEMORY_MAP_ADDRESS
xor ebx, ebx ; Continuation value
loop:
mov ecx, ADDRESS_RANGE_DESCRIPTOR_SIZE
mov edx, SYSTEM_MAP_SIGNATURE
mov eax, SYSTEM_ADDRESS_MAP_FUNCTION_CODE
int BIOS_SYSTEM_SERVICES
jc error
test ebx, ebx
jz ok
add edi, ADDRESS_RANGE_DESCRIPTOR_SIZE
jmp loop

; Load kernel.
mov dl, DISK
xor ax, ax
mov ds, ax
mov si, kernel_disk_address_packet
mov ah, EXTENDED_READ_FUNCTION_CODE
int BIOS_DISK_SERVICES
jc error

error:
mov ax, video_segment
mov es, ax
xor di, di
mov byte [es:di], 'e'
mov byte [es:di + 1], yellow_on_magenta
jmp done

ok:
mov ax, video_segment
mov es, ax
xor di, di
mov byte [es:di], 'K'
mov byte [es:di + 1], green_on_black

cli
lgdt [GDT_descriptor]
lidt [IDT_descriptor]

mov eax, cr0
or eax, PROTECTED_MODE
mov cr0, eax


jmp code_selector:protected_mode_start


[BITS 32]
protected_mode_start:

mov ax, data_selector
mov ds, ax
mov es, ax
mov ss, ax

mov esp, MBR_ADDRESS

mov byte [VIDEO_ADDRESS], 'P'
mov byte [VIDEO_ADDRESS + 1], grey_on_black


done:
hlt
jmp done


; For reading kernel into memory.
kernel_disk_address_packet:
db DISK_ADDRESS_PACKET_SIZE
db 0
dw NUM_OF_KERNEL_SECTORS_TO_READ
dw kernel_offset, kernel_segment
dq kernel_start_sector



global_descriptor_table:
dq NULL_SEGMENT

; Code segment.
dw SEGMENT_LIMIT & 0xffff
dw BASE_ADDRESS & 0xffff
db BASE_ADDRESS >> 16 & 0xff
db code_access_byte
db flags_nibble << 4 | SEGMENT_LIMIT >> 16
db BASE_ADDRESS >> 24

; Data segment.
dw SEGMENT_LIMIT & 0xffff
dw BASE_ADDRESS & 0xffff
db BASE_ADDRESS >> 16 & 0xff
db data_access_byte
db flags_nibble << 4 | SEGMENT_LIMIT >> 16
db BASE_ADDRESS >> 24


GDT_size_minus_1 equ $ - global_descriptor_table - 1


GDT_descriptor:
dw GDT_size_minus_1
dd global_descriptor_table


IDT_descriptor:
dw IDT_INVALID_SIZE_MINUS_1
dd INTERRUPT_DESCRIPTOR_TABLE_INVALID_ADDRESS
