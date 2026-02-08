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

; Loader for Toucanix.

%include "defs.inc"


BIOS_SYSTEM_SERVICES equ 0x15
SYSTEM_PA_MAP_FUNCTION_CODE equ 0xe820
; SMAP. PAMS in little-endian.
SYSTEM_MAP_SIGNATURE equ 0x534D4150
PA_RANGE_DESCRIPTOR_SIZE equ 20


; cpuid.
GET_HIGHEST_EXTENDED_FUNCTION equ 0x8000_0000
GET_EXTENDED_FEATURES         equ 0x8000_0001

GIGABYTE_PAGE_SUPPORT equ 1 << 26
LONG_MODE_SUPPORT     equ 1 << 29


; PM = Protected Mode.

CODE_ACCESS_BYTE_PM equ PRESENT_BIT_SET \
    | CODE_OR_DATA_SEGMENT_TYPE         \
    | EXEC                              \
    | CODE_READ_OR_DATA_WRITE_ACCESS

DATA_ACCESS_BYTE_PM equ PRESENT_BIT_SET \
    | CODE_OR_DATA_SEGMENT_TYPE         \
    | CODE_READ_OR_DATA_WRITE_ACCESS

FLAGS_NIBBLE_PM equ GRANULARITY_4_KIB \
    | SIZE_32_BIT_SEGMENT


BASE_PA equ 0

SEGMENT_LIMIT_PM equ 0xfffff
SEGMENT_LIMIT equ 0

DATA_SEGMENT_INDEX equ 2
DATA_SELECTOR equ DATA_SEGMENT_INDEX << 3

; IDT = Interrupt Descriptor Table.
IDT_PM_INVALID_SIZE_MINUS_1 equ 0
ZERO_PA equ 0
IDT_PM_INVALID_PA equ ZERO_PA

PROTECTED_MODE equ 1


; Model-Specific Register: Extended Feature Enable Register.
MSR_EFER equ 0xC0000080

LONG_MODE_ENABLE equ 1 << 8
PA_EXTENSION equ 1 << 5
PAGING equ 1 << 31


; PML4 = Page Map Level 4 (table).
; PDPT = Page Directory Pointer Table.

LOWER_BIT_OF_PML4_COMPONENT equ 39
LOWER_9_BITS equ 0x1ff

PML4E_KERNEL_SPACE equ PML4_PA \
    + (KERNEL_SPACE_VA >> LOWER_BIT_OF_PML4_COMPONENT & LOWER_9_BITS) \
    * BYTES_PER_PAGE_TABLE_ENTRY


[BITS 16]
[ORG LOADER_PA]

; Save the system memory map.
mov dword [MEMORY_MAP_ENTRY_COUNT_PA], 0
mov edi, MEMORY_MAP_PA
xor ebx, ebx ; Continuation value
mm_loop:
mov ecx, PA_RANGE_DESCRIPTOR_SIZE
mov edx, SYSTEM_MAP_SIGNATURE
mov eax, SYSTEM_PA_MAP_FUNCTION_CODE
int BIOS_SYSTEM_SERVICES
jc error_a
inc dword [MEMORY_MAP_ENTRY_COUNT_PA]
test ebx, ebx
jz ok
add edi, PA_RANGE_DESCRIPTOR_SIZE
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


; Load user A bin.
mov dl, DISK
xor ax, ax
mov ds, ax
mov si, user_a_disk_address_packet
mov ah, EXTENDED_READ_FUNCTION_CODE
int BIOS_DISK_SERVICES
jc error_f


; Load user B bin.
mov dl, DISK
xor ax, ax
mov ds, ax
mov si, user_b_disk_address_packet
mov ah, EXTENDED_READ_FUNCTION_CODE
int BIOS_DISK_SERVICES
jc error_g


kernel_loaded:

; Prepare for Protected Mode.
cli
lgdt [GDT_PM_descriptor]
lidt [IDT_PM_descriptor]

mov eax, cr0
or eax, PROTECTED_MODE
mov cr0, eax

jmp CODE_SELECTOR:protected_mode_start


; Error cases.
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
error_f:
    mov si, user_a_load_failed
    jmp error
error_g:
    mov si, user_b_load_failed
    jmp error


error:
xor ax, ax
mov ds, ax
mov bl, YELLOW_ON_MAGENTA
call PRINT_FUNC
.done:
hlt
jmp .done




[BITS 32]
protected_mode_start:

mov ax, DATA_SELECTOR
mov ds, ax
mov es, ax
mov ss, ax

mov esp, MBR_PA


setup_paging:
; Zero data.
cld
mov edi, PML4_PA
xor eax, eax
; Clear enough space to fit the PML4 and one PDPT.
mov ecx, PAGE_TABLE_SIZE * 2 / DWORD_SIZE
rep stosd

; The PML4 entry for the identity mapping is the first one.
mov dword [PML4_PA], \
    PDPT_PA | READ_AND_WRITE | PAGE_PRESENT

mov dword [PML4E_KERNEL_SPACE], \
    PDPT_PA | READ_AND_WRITE | PAGE_PRESENT

; The same PDPT is used for both the identity and the kernel space mappings.
; There is only one PML4. Each PML4E (E for entry) points to a PDPT.
; In this case both the identity and kernel space PML4E's point to the
; same PDPT (the other PML4E's are not used).
; Now, each PDPTE points to one GiB of memory. So, by filling
; in a complete PDPT, 512GiB of memory will be mapped (completed in a later
; step). The first 512GiB of physical memory is identity mapped and mapped
; into the kernel space.

; First GiB only.
mov dword [PDPT_PA], \
    ZERO_PA | PS | READ_AND_WRITE | PAGE_PRESENT

mov eax, PML4_PA
mov cr3, eax


; Prepare for Long Mode.
lgdt [GDT_descriptor]

mov ecx, MSR_EFER
rdmsr
or eax, LONG_MODE_ENABLE
wrmsr

mov eax, cr4
or eax, PA_EXTENSION
mov cr4, eax

mov eax, cr0
or eax, PAGING
mov cr0, eax

jmp CODE_SELECTOR:long_mode_start


[BITS 64]
long_mode_start:

mov rsp, MBR_PA


complete_paging:
mov rcx, 1 ; First GiB has already been done.
mov rdi, PDPT_PA + BYTES_PER_PAGE_TABLE_ENTRY
xor rsi, rsi

.loop:
cmp rcx, NUM_GIB_MAPPED
jae .done
mov rsi, rcx
shl rsi, EXP_1_GIB
or rsi, PS | READ_AND_WRITE | PAGE_PRESENT
mov [rdi], rsi
add rdi, BYTES_PER_PAGE_TABLE_ENTRY
inc rcx
jmp .loop
.done:


; Relocate the kernel.
cld
mov rdi, KERNEL_PA
mov rsi, KERNEL_ORIGINAL_PA
mov rcx, KERNEL_SIZE / 8
rep movsq

mov rax, KERNEL_VA
jmp rax


; Data.

memory_map_failed: db 'ERROR: Failed to get memory map', NL, 0
no_extended_features: db 'ERROR: No extended features available', NL, 0
no_long_mode: db 'ERROR: No long mode support', NL, 0
no_gigabyte_page: db 'ERROR: No gigabyte page support', NL, 0
kernel_load_failed: db 'ERROR: Failed to load kernel', NL, 0
user_a_load_failed: db 'ERROR: Failed to load user A', NL, 0
user_b_load_failed: db 'ERROR: Failed to load user B', NL, 0


; For reading kernel into memory.
kernel_disk_address_packet:
db DISK_PA_PACKET_SIZE
db 0
dw KERNEL_SECTORS
dw KERNEL_ORIGINAL_OFFSET, KERNEL_ORIGINAL_SEGMENT
dq KERNEL_START_SECTOR


; For reading user A bin into memory.
user_a_disk_address_packet:
db DISK_PA_PACKET_SIZE
db 0
dw USER_A_SECTORS
dw USER_A_OFFSET, USER_A_SEGMENT
dq USER_A_START_SECTOR

; For reading user B bin into memory.
user_b_disk_address_packet:
db DISK_PA_PACKET_SIZE
db 0
dw USER_B_SECTORS
dw USER_B_OFFSET, USER_B_SEGMENT
dq USER_B_START_SECTOR




global_descriptor_table_PM:
dq NULL_SEGMENT

; Code segment.
dw SEGMENT_LIMIT_PM & 0xffff
dw BASE_PA & 0xffff
db BASE_PA >> 16 & 0xff
db CODE_ACCESS_BYTE_PM
db FLAGS_NIBBLE_PM << 4 | SEGMENT_LIMIT_PM >> 16
db BASE_PA >> 24

; Data segment.
dw SEGMENT_LIMIT_PM & 0xffff
dw BASE_PA & 0xffff
db BASE_PA >> 16 & 0xff
db DATA_ACCESS_BYTE_PM
db FLAGS_NIBBLE_PM << 4 | SEGMENT_LIMIT_PM >> 16
db BASE_PA >> 24

GDT_PM_SIZE equ $ - global_descriptor_table_PM



GDT_PM_descriptor:
dw GDT_PM_SIZE - 1
dd global_descriptor_table_PM


IDT_PM_descriptor:
dw IDT_PM_INVALID_SIZE_MINUS_1
; This expects a virtual address, but since there is currently an identity
; mapping in-force, then a "physical" address can be used.
dd IDT_PM_INVALID_PA



global_descriptor_table:
dq NULL_SEGMENT

; Code segment. Base and limit are ignored.
dw 0, 0
db 0, CODE_ACCESS_BYTE, LONG_MODE_CODE << 4, 0

LOADER_GDT_SIZE equ $ - global_descriptor_table


GDT_descriptor:
dw LOADER_GDT_SIZE - 1
dd global_descriptor_table
