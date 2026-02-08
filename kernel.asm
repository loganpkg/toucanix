;
; Copyright (c) 2025, 2026 Logan Ryan McLintock. All rights reserved.
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

; Kernel for Toucanix.

%include "defs.inc"


; Programmable Interval Timer (PIT).
RATE_GENERATOR equ 2 << 1
LOW_BYTE_THEN_HIGH_BYTE equ 3 << 4
PIT_COMMAND_PORT equ 0x43
PIT_DATA_PORT equ 0x40 ; Channel 0.
OSCILLATOR_FREQUENCY_HZ equ 1193182
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
TSS_INDEX equ 4

TSS_SELECTOR equ TSS_INDEX << 3

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
    | CODE_OR_DATA_SEGMENT_TYPE | CODE_READ_OR_DATA_WRITE_ACCESS, 0, 0

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

KERNEL_GDT_SIZE equ $ - global_descriptor_table


GDT_descriptor:
dw KERNEL_GDT_SIZE - 1
dq global_descriptor_table



; Task state segment.
tss:
dd 0
dq KERNEL_STACK_VA
times TSS_SIZE - ($ - tss) - DWORD_SIZE db 0
dd TSS_SIZE




section .text
extern kernel_main
global _start

_start:

mov rsp, KERNEL_STACK_VA

; Remove identity mapping.
; Clear the PML4E (note the E for Entry) that is being used for the identity
; mapping. The PML4E that is used for the kernel space is still in force.
; That is, the first 512GiB of physical memory is still mapped to the kernel
; space.
mov rax, PML4_VA
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
