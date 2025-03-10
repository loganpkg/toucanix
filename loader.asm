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

; Loader for Toucanix.

%include "defs.asm"

; System Memory Map.
MEMORY_MAP_ENTRY_COUNT_ADDRESS equ 0x9000
DWORD_SIZE equ 4
MEMORY_MAP_ADDRESS equ MEMORY_MAP_ENTRY_COUNT_ADDRESS + DWORD_SIZE

BIOS_SYSTEM_SERVICES equ 0x15
SYSTEM_ADDRESS_MAP_FUNCTION_CODE equ 0xe820
; SMAP. PAMS in little-endian.
SYSTEM_MAP_SIGNATURE equ 0x534D4150
ADDRESS_RANGE_DESCRIPTOR_SIZE equ 20


; cpuid.
GET_HIGHEST_EXTENDED_FUNCTION equ 0x8000_0000
GET_EXTENDED_FEATURES         equ 0x8000_0001

GIGABYTE_PAGE_SUPPORT equ 1 << 26
LONG_MODE_SUPPORT     equ 1 << 29


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
; PDPT = Page Directory Pointer Table.
ZERO_ADDRESS equ 0
PML4_SIZE equ 0x1000
PDPT_SIZE equ 0x1000
PDPT_ADDRESS equ PML4_ADDRESS + PML4_SIZE

LOWER_BIT_OF_PML4_COMPONENT equ 39
LOWER_9_BITS equ 0x1ff
BYTES_PER_PML4_ENTRY equ 8

PML4E_KERNEL_SPACE equ PML4_ADDRESS \
    + (KERNEL_SPACE_VIRTUAL_ADDRESS >> LOWER_BIT_OF_PML4_COMPONENT \
        & LOWER_9_BITS) * BYTES_PER_PML4_ENTRY

BYTES_PER_PDPT_ENTRY equ 8
NUM_GIB_MAPPED equ PDPT_SIZE / BYTES_PER_PDPT_ENTRY
EXPONENT_1_GIB equ 30

PAGE_PRESENT    equ 1
READ_AND_WRITE  equ 1 << 1
USER_ACCESS     equ 1 << 2
; Page Size attribute.
PS              equ 1 << 7




[BITS 16]
[ORG LOADER_ADDRESS]

; Save the system memory map.
mov dword [MEMORY_MAP_ENTRY_COUNT_ADDRESS], 0
mov edi, MEMORY_MAP_ADDRESS
xor ebx, ebx ; Continuation value
mm_loop:
mov ecx, ADDRESS_RANGE_DESCRIPTOR_SIZE
mov edx, SYSTEM_MAP_SIGNATURE
mov eax, SYSTEM_ADDRESS_MAP_FUNCTION_CODE
int BIOS_SYSTEM_SERVICES
jc error_a
inc dword [MEMORY_MAP_ENTRY_COUNT_ADDRESS]
test ebx, ebx
jz ok
add edi, ADDRESS_RANGE_DESCRIPTOR_SIZE
jmp mm_loop
ok:


mov eax, GET_HIGHEST_EXTENDED_FUNCTION
cpuid
cmp eax, GET_EXTENDED_FEATURES
jb error_b

mov eax, GET_EXTENDED_FEATURES
cpuid
test edx, LONG_MODE_SUPPORT
jz error_c

test edx, GIGABYTE_PAGE_SUPPORT
jz error_d


; Load kernel.
mov dl, DISK
xor ax, ax
mov ds, ax
mov si, kernel_disk_address_packet
mov ah, EXTENDED_READ_FUNCTION_CODE
int BIOS_DISK_SERVICES
jc error_e
jmp kernel_loaded


error_a:
mov si, memory_map_failed
jmp error
error_b:
mov si, no_extended_features
jmp error
error_c:
mov si, no_long_mode
jmp error
error_d:
mov si, no_gigabyte_page
jmp error
error_e:
mov si, kernel_load_failed
jmp error

error:
xor ax, ax
mov ds, ax
mov bl, YELLOW_ON_MAGENTA
call PRINT_FUNC
.done:
hlt
jmp .done


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


setup_paging:
; Zero data.
cld
mov edi, PML4_ADDRESS
xor eax, eax
mov ecx, (PML4_SIZE + PDPT_SIZE) / BYTES_PER_DOUBLE_WORD
rep stosd


mov dword [PML4E_IDENTITY], \
    PDPT_ADDRESS | READ_AND_WRITE | PAGE_PRESENT

mov dword [PML4E_KERNEL_SPACE], \
    PDPT_ADDRESS | READ_AND_WRITE | PAGE_PRESENT

; First GiB only.
mov dword [PDPT_ADDRESS], \
    ZERO_ADDRESS | PS | READ_AND_WRITE | PAGE_PRESENT

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


complete_paging:
mov rcx, 1 ; First GiB has already been done.
mov rdi, PDPT_ADDRESS + BYTES_PER_PDPT_ENTRY
xor rsi, rsi

.loop:
cmp rcx, NUM_GIB_MAPPED
jae .done
mov rsi, rcx
shl rsi, EXPONENT_1_GIB
or rsi, PS | READ_AND_WRITE | PAGE_PRESENT
mov [rdi], rsi
add rdi, BYTES_PER_PDPT_ENTRY
inc rcx
jmp .loop
.done:


; Relocate the kernel.
cld
mov rdi, KERNEL_ADDRESS
mov rsi, KERNEL_ORIGINAL_ADDRESS
mov rcx, KERNEL_SIZE / 8
rep movsq

mov rax, KERNEL_VIRTUAL_ADDRESS
jmp rax


; Data.

memory_map_failed: db 'ERROR: Failed to get memory map', NL, 0
no_extended_features: db 'ERROR: No extended features available', NL, 0
no_long_mode: db 'ERROR: No long mode support', NL, 0
no_gigabyte_page: db 'ERROR: No gigabyte page support', NL, 0
kernel_load_failed: db 'ERROR: Failed to load kernel', NL, 0


; For reading kernel into memory.
kernel_disk_address_packet:
db DISK_ADDRESS_PACKET_SIZE
db 0
dw KERNEL_SECTORS
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
