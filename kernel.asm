;
; Copyright (c) 2025 Logan Ryan McLintock
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

; Kernel for Toucanix.

%include "defs.inc"

KERNEL_STACK_VA equ KERNEL_VA


; Programmable Interval Timer (PIT).
RATE_GENERATOR equ 2 << 1
LOW_BYTE_THEN_HIGH_BYTE equ 3 << 4
PIT_COMMAND_PORT equ 0x43
PIT_DATA_PORT equ 0x40 ; Channel 0.
OSCILLATOR_FREQUENCY_HZ equ 1193182
EVENTS_PER_SECOND equ 100
PIT_COUNTER equ OSCILLATOR_FREQUENCY_HZ / EVENTS_PER_SECOND


; Programmable Interrupt Controller (PIC).
; IRQ = Interrupt ReQuest.
PIC_MASTER_DATA     equ PIC_MASTER_COMMAND + 1
PIC_SLAVE_COMMAND   equ 0xa0
PIC_SLAVE_DATA      equ PIC_SLAVE_COMMAND + 1
INIT_COMMAND        equ 1 << 4
FOUR_BYTE_INIT_SEQ  equ 1
IRQ_0_MAP           equ 32
IRQ_7_MAP           equ IRQ_0_MAP + 7
NUM_IRQ_ON_MASTER   equ 8
IRQ_8_MAP           equ IRQ_0_MAP + NUM_IRQ_ON_MASTER
SLAVE_TO_MASTER_IQR equ 2
MODE_8086           equ 1
ONLY_IQR_0_ENABLED  equ 0xe
MASK_ALL            equ 0xf



; GDT.
USER_CODE_SEGMENT_INDEX equ 2
USER_DATA_SEGMENT_INDEX equ 3
TSS_INDEX equ 4

USER_CODE_SELECTOR equ USER_CODE_SEGMENT_INDEX << 3 | USER_RING
USER_DATA_SELECTOR equ USER_DATA_SEGMENT_INDEX << 3 | USER_RING
TSS_SELECTOR equ TSS_INDEX << 3

RFLAGS_INTERRUPT_ENABLE equ 1 << 9
RFLAGS_RESERVED_BIT_1 equ 1 << 1


TSS_SIZE equ 104

TSS_AVAILABLE equ 9




section .data
global tss

global_descriptor_table:
; Base and limit are ignored (except for the TSS).
dq NULL_SEGMENT

; Code segment for kernel.
dw 0, 0
db 0, CODE_ACCESS_BYTE, LONG_MODE_CODE << 4, 0

; Code segment for user.
dw 0, 0
db 0, CODE_ACCESS_BYTE | DESCRIPTOR_PRIVILEGE_LEVEL_USER, \
    LONG_MODE_CODE << 4, 0

; Data segment for user.
dw 0, 0
db 0, PRESENT_BIT_SET | DESCRIPTOR_PRIVILEGE_LEVEL_USER \
    | TYPE_IS_CODE_OR_DATA_SEGMENT | CODE_READ_OR_DATA_WRITE_ACCESS, 0, 0

tss_entry: ; Double the usual GDT entry size.
dw TSS_SIZE - 1 & 0xffff
.base_a_word:
dw 0
.base_b_byte:
db 0
db PRESENT_BIT_SET | TSS_AVAILABLE
db TSS_SIZE - 1 >> 16
.base_c_byte:
db 0
.base_d_dword:
dd 0
dd 0

GDT_SIZE equ $ - global_descriptor_table


GDT_descriptor:
dw GDT_SIZE - 1
dq global_descriptor_table



; Task state segment.
tss:
dd 0
dq KERNEL_STACK_VA
times TSS_SIZE - ($ - tss) - BYTES_PER_DOUBLE_WORD db 0
dd TSS_SIZE




section .text
extern kernel_main
global _start

_start:

mov rsp, KERNEL_STACK_VA

; Remove identity mapping.
mov rax, PML4E_IDENTITY_VA
mov qword [rax], 0


; Complete TSS entry in GDT.
mov rax, qword tss

mov rbx, qword tss_entry.base_a_word
mov [rbx], ax

shr rax, 16

mov rbx, qword tss_entry.base_b_byte
mov [rbx], al

shr rax, 8

mov rbx, qword tss_entry.base_c_byte
mov [rbx], al

shr rax, 8

mov rbx, qword tss_entry.base_d_dword
mov [rbx], eax

mov rax, qword GDT_descriptor
lgdt [rax]
mov ax, TSS_SELECTOR
ltr ax


; Programmable Interval Timer.
mov al, RATE_GENERATOR | LOW_BYTE_THEN_HIGH_BYTE
out PIT_COMMAND_PORT, al

mov ax, PIT_COUNTER
out PIT_DATA_PORT, al
shr ax, 8
out PIT_DATA_PORT, al


; Initialise the Programmable Interrupt Controller (PIC).
; Master.
mov al, INIT_COMMAND | FOUR_BYTE_INIT_SEQ
out PIC_MASTER_COMMAND, al
mov al, IRQ_0_MAP
out PIC_MASTER_DATA, al
mov al, 1 << SLAVE_TO_MASTER_IQR
out PIC_MASTER_DATA, al
mov al, MODE_8086
out PIC_MASTER_DATA, al

mov al, ONLY_IQR_0_ENABLED
out PIC_MASTER_DATA, al


; Slave
mov al, INIT_COMMAND | FOUR_BYTE_INIT_SEQ
out PIC_SLAVE_COMMAND, al
mov al, IRQ_8_MAP
out PIC_SLAVE_DATA, al
mov al, SLAVE_TO_MASTER_IQR
out PIC_SLAVE_DATA, al
mov al, MODE_8086
out PIC_SLAVE_DATA, al

mov al, MASK_ALL
out PIC_SLAVE_DATA, al


; Set cs register.
push CODE_SELECTOR
mov rax, qword kernel_start
push rax
retfq


kernel_start:
call kernel_main

.done:
hlt
jmp .done
