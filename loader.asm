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
kernel_start_sector equ MBR_SECTOR + NUM_OF_LOADER_SECTORS_TO_READ
%define NUM_OF_KERNEL_SECTORS_TO_READ 120

code_access_byte equ PRESENT_BIT_SET \
    | TYPE_IS_CODE_OR_DATA_SEGMENT   \
    | EXECUTABLE                     \
    | CODE_READ_OR_DATA_WRITE_ACCESS

data_access_byte equ PRESENT_BIT_SET \
    | TYPE_IS_CODE_OR_DATA_SEGMENT   \
    | CODE_READ_OR_DATA_WRITE_ACCESS

flags_nibble equ GRANULARITY_4_KIB \
    | SIZE_32_BIT_SEGMENT


%define BASE_ADDRESS 0
; %define BASE_ADDRESS 0xefcdab89

%define SEGMENT_LIMIT 0xfffff
%define SEGMENT_LIMIT_64 0

%define DATA_SEGMENT_INDEX 2
data_selector equ DATA_SEGMENT_INDEX << 3


%define IDT_INVALID_SIZE_MINUS_1 0
%define INTERRUPT_DESCRIPTOR_TABLE_INVALID_ADDRESS 0

%define PROTECTED_MODE 1


; Model-Specific Register: Extended Feature Enable Register.
%define MSR_EFER 0xC0000080

%define LONG_MODE_ENABLE (1 << 8)
%define PHYSICAL_ADDRESS_EXTENSION (1 << 5)
%define PAGING (1 << 31)


; PML4 = Page Map Level 4 (table).
; PDP = Page Directory Pointer (table).
%define PML4_ADDRESS 0x70000
%define PML4_SIZE 0x1000
PDP_address equ PML4_ADDRESS + PML4_SIZE
%define PDP_SIZE 0x1000

%define BYTES_PER_DOUBLE_WORD 4

%define PAGE_PRESENT 1
%define READ_AND_WRITE (1 << 2)
%define USER_ACCESS (1 << 3)
%define GIB_SIZE (1 << 7)


[BITS 16]
[ORG loader_address]

; Save the system memory map.
mov edi, SYSTEM_MEMORY_MAP_ADDRESS
xor ebx, ebx ; Continuation value
mm_loop:
mov ecx, ADDRESS_RANGE_DESCRIPTOR_SIZE
mov edx, SYSTEM_MAP_SIGNATURE
mov eax, SYSTEM_ADDRESS_MAP_FUNCTION_CODE
int BIOS_SYSTEM_SERVICES
jc error
test ebx, ebx
jz ok
add edi, ADDRESS_RANGE_DESCRIPTOR_SIZE
jmp mm_loop
ok:

; Load kernel.
mov dl, DISK
xor ax, ax
mov ds, ax
mov si, kernel_disk_address_packet
mov ah, EXTENDED_READ_FUNCTION_CODE
int BIOS_DISK_SERVICES
jc error
jmp kernel_loaded

error:
mov ax, video_segment
mov es, ax
xor di, di
mov byte [es:di], 'e'
mov byte [es:di + 1], yellow_on_magenta
done:
hlt
jmp done

kernel_loaded:

; Prepare for Protected Mode.
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


; Setup paging.
; Zero data.
cld
mov edi, PML4_ADDRESS
xor eax, eax
mov ecx, (PML4_SIZE + PDP_SIZE) / BYTES_PER_DOUBLE_WORD
rep stosd

mov dword [PML4_ADDRESS], \
    PDP_address | USER_ACCESS | READ_AND_WRITE | PAGE_PRESENT

; Identity map.
mov dword [PDP_address], \
    GIB_SIZE | USER_ACCESS | READ_AND_WRITE | PAGE_PRESENT

mov eax, PML4_ADDRESS
mov cr3, eax


; Prepare for Long Mode.
lgdt [GDT_descriptor_64]

mov ecx, MSR_EFER
rdmsr
or eax, LONG_MODE_ENABLE
wrmsr

mov eax, cr4
or eax, PHYSICAL_ADDRESS_EXTENSION
mov cr4, eax

mov eax, cr0
or eax, PAGING
mov cr0, eax

jmp code_selector:long_mode_start


[BITS 64]
long_mode_start:

mov rsp, MBR_ADDRESS

; Relocate the kernel.
cld
mov rdi, KERNEL_NEW_ADDRESS
mov rsi, KERNEL_ADDRESS
mov rcx, kernel_size / 8
rep movsq

jmp KERNEL_NEW_ADDRESS


; Data.

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



global_descriptor_table_64:
dq NULL_SEGMENT

; Code segment. Base and limit are ignored.
dw 0, 0
db 0, code_access_byte_64, flags_nibble_64 << 4, 0

GDT_64_size_minus_1 equ $ - global_descriptor_table_64 - 1


GDT_descriptor_64:
dw GDT_64_size_minus_1
dd global_descriptor_table_64
