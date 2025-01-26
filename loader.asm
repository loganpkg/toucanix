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

SYSTEM_MEMORY_MAP_ADDRESS equ 0x9000
BIOS_SYSTEM_SERVICES equ 0x15
SYSTEM_ADDRESS_MAP_FUNCTION_CODE equ 0xe820
; SMAP. PAMS in little-endian.
SYSTEM_MAP_SIGNATURE equ 0x534D4150
ADDRESS_RANGE_DESCRIPTOR_SIZE equ 20
KERNEL_START_SECTOR equ MBR_SECTOR + NUM_OF_LOADER_SECTORS_TO_READ

; PM = Protected Mode.

CODE_ACCESS_BYTE_PM equ PRESENT_BIT_SET \
    | TYPE_IS_CODE_OR_DATA_SEGMENT   \
    | EXECUTABLE                     \
    | CODE_READ_OR_DATA_WRITE_ACCESS

DATA_ACCESS_BYTE_PM equ PRESENT_BIT_SET \
    | TYPE_IS_CODE_OR_DATA_SEGMENT   \
    | CODE_READ_OR_DATA_WRITE_ACCESS

FLAGS_NIBBLE_PM equ GRANULARITY_4_KIB \
    | SIZE_32_BIT_SEGMENT


BASE_ADDRESS equ 0

SEGMENT_LIMIT_PM equ 0xfffff
SEGMENT_LIMIT equ 0

DATA_SEGMENT_INDEX equ 2
DATA_SELECTOR equ DATA_SEGMENT_INDEX << 3

; IDT = Interrupt Descriptor Table.
IDT_PM_INVALID_SIZE_MINUS_1 equ 0
IDT_PM_INVALID_ADDRESS equ 0

PROTECTED_MODE equ 1


; Model-Specific Register: Extended Feature Enable Register.
MSR_EFER equ 0xC0000080

LONG_MODE_ENABLE equ 1 << 8
PHYSICAL_ADDRESS_EXTENSION equ 1 << 5
PAGING equ 1 << 31


; PML4 = Page Map Level 4 (table).
; PDP = Page Directory Pointer (table).
PML4_ADDRESS equ 0x70000
PML4_SIZE equ 0x1000
PDP_ADDRESS equ PML4_ADDRESS + PML4_SIZE
PDP_SIZE equ 0x1000


PAGE_PRESENT equ 1
READ_AND_WRITE equ 1 << 1
USER_ACCESS equ 1 << 2
GIB_SIZE equ 1 << 7


[BITS 16]
[ORG LOADER_ADDRESS]

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
mov ax, VIDEO_SEGMENT
mov es, ax
xor di, di
mov byte [es:di], 'e'
mov byte [es:di + 1], YELLOW_ON_MAGENTA
done:
hlt
jmp done

kernel_loaded:

; Prepare for Protected Mode.
cli
lgdt [GDT_PM_descriptor]
lidt [IDT_PM_descriptor]

mov eax, cr0
or eax, PROTECTED_MODE
mov cr0, eax

jmp CODE_SELECTOR:protected_mode_start


[BITS 32]
protected_mode_start:

mov ax, DATA_SELECTOR
mov ds, ax
mov es, ax
mov ss, ax

mov esp, MBR_ADDRESS


; Setup paging.
; Zero data.
cld
mov edi, PML4_ADDRESS
xor eax, eax
mov ecx, (PML4_SIZE + PDP_SIZE) / BYTES_PER_DOUBLE_WORD
rep stosd

jmp cool
db 'ELEPHANT'
cool:

mov dword [PML4_ADDRESS], \
    PDP_ADDRESS | USER_ACCESS | READ_AND_WRITE | PAGE_PRESENT

; Identity map.
mov dword [PDP_ADDRESS], \
    GIB_SIZE | USER_ACCESS | READ_AND_WRITE | PAGE_PRESENT

jmp cool2
db 'ELEPHANT'
cool2:


mov eax, PML4_ADDRESS
mov cr3, eax


; Prepare for Long Mode.
lgdt [GDT_descriptor]

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

jmp CODE_SELECTOR:long_mode_start


[BITS 64]
long_mode_start:

mov rsp, MBR_ADDRESS

; Relocate the kernel.
cld
mov rdi, KERNEL_ADDRESS
mov rsi, KERNEL_ORIGINAL_ADDRESS
mov rcx, KERNEL_SIZE / 8
rep movsq

jmp KERNEL_ADDRESS


; Data.

; For reading kernel into memory.
kernel_disk_address_packet:
db DISK_ADDRESS_PACKET_SIZE
db 0
dw NUM_OF_KERNEL_SECTORS_TO_READ
dw KERNEL_ORIGINAL_OFFSET, KERNEL_ORIGINAL_SEGMENT
dq KERNEL_START_SECTOR


global_descriptor_table_PM:
dq NULL_SEGMENT

; Code segment.
dw SEGMENT_LIMIT_PM & 0xffff
dw BASE_ADDRESS & 0xffff
db BASE_ADDRESS >> 16 & 0xff
db CODE_ACCESS_BYTE_PM
db FLAGS_NIBBLE_PM << 4 | SEGMENT_LIMIT_PM >> 16
db BASE_ADDRESS >> 24

; Data segment.
dw SEGMENT_LIMIT_PM & 0xffff
dw BASE_ADDRESS & 0xffff
db BASE_ADDRESS >> 16 & 0xff
db DATA_ACCESS_BYTE_PM
db FLAGS_NIBBLE_PM << 4 | SEGMENT_LIMIT_PM >> 16
db BASE_ADDRESS >> 24

GDT_PM_SIZE equ $ - global_descriptor_table_PM


GDT_PM_descriptor:
dw GDT_PM_SIZE - 1
dd global_descriptor_table_PM


IDT_PM_descriptor:
dw IDT_PM_INVALID_SIZE_MINUS_1
dd IDT_PM_INVALID_ADDRESS



global_descriptor_table:
dq NULL_SEGMENT

; Code segment. Base and limit are ignored.
dw 0, 0
db 0, CODE_ACCESS_BYTE, LONG_MODE_CODE << 4, 0

GDT_SIZE equ $ - global_descriptor_table


GDT_descriptor:
dw GDT_SIZE - 1
dd global_descriptor_table
