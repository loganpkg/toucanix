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

; Kernel for Toucanix.

%include "defs.asm"

IDT_NUM_ENTRIES equ 256
IDT_ENTRY_SIZE equ 16
INTERRUPT_GATE_TYPE equ 0xe
OFFSET_16_TO_31 equ 6
OFFSET_32_TO_63 equ 8

; Programmable Interval Timer (PIT).
RATE_GENERATOR equ 2 << 1
LOW_BYTE_THEN_HIGH_BYTE equ 3 << 4
PIT_COMMAND_PORT equ 0x43
PIT_DATA_PORT equ 0x40 ; Channel 0.
OSCILLATOR_FREQUENCY_HZ equ 1193182
EVENTS_PER_SECOND equ 100
PIT_COUNTER equ OSCILLATOR_FREQUENCY_HZ / EVENTS_PER_SECOND


; Programmable Interrupt Controller (PIC).
PIC_MASTER_COMMAND  equ 0x20
PIC_MASTER_DATA     equ PIC_MASTER_COMMAND + 1
PIC_SLAVE_COMMAND   equ 0xa0
PIC_SLAVE_DATA      equ PIC_SLAVE_COMMAND + 1
INIT_COMMAND        equ 1 << 4
FOUR_BYTE_INIT_SEQ  equ 1
IRQ_0_MAP           equ 32
NUM_IRQ_ON_MASTER   equ 8
IRQ_8_MAP           equ IRQ_0_MAP + NUM_IRQ_ON_MASTER
SLAVE_TO_MASTER_IQR equ 2
MODE_8086           equ 1
ONLY_IQR_0_ENABLED  equ 0xe
MASK_ALL            equ 0xf


[BITS 64]
[ORG KERNEL_NEW_ADDRESS]

mov rax, divide_by_zero
mov rdi, interrupt_descriptor_table_64
mov [rdi], ax       ; Offset: Lower two bytes.
shr rax, 16
mov [rdi + OFFSET_16_TO_31], ax
shr rax, 16
mov [rdi + OFFSET_32_TO_63], eax


mov rax, interval_timer
mov rdi, interrupt_descriptor_table_64 + IRQ_0_MAP * IDT_ENTRY_SIZE
mov [rdi], ax       ; Offset: Lower two bytes.
shr rax, 16
mov [rdi + OFFSET_16_TO_31], ax
shr rax, 16
mov [rdi + OFFSET_32_TO_63], eax


lgdt [GDT_descriptor_64]
lidt [IDT_descriptor_64]

; Set cs register.
push CODE_SELECTOR
push kernel_start
retfq

kernel_start:

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


sti

; xor rbx, rbx
; div rbx


done:
hlt
jmp done


divide_by_zero:
%include "push_all.asm"
mov byte [VIDEO_ADDRESS], 'Z'
mov byte [VIDEO_ADDRESS + 1], RED_ON_BLACK
jmp done
%include "pop_all.asm"
iretq


interval_timer:
%include "push_all.asm"
mov byte [VIDEO_ADDRESS], 'I'
mov byte [VIDEO_ADDRESS + 1], GREEN_ON_BLACK
jmp done
%include "pop_all.asm"
iretq


; Data.

global_descriptor_table_64:
dq NULL_SEGMENT

; Code segment. Base and limit are ignored.
dw 0, 0
db 0, CODE_ACCESS_BYTE_64, FLAGS_NIBBLE_64 << 4, 0

GDT_64_SIZE_MINUS_1 equ $ - global_descriptor_table_64 - 1


GDT_descriptor_64:
dw GDT_64_SIZE_MINUS_1
dq global_descriptor_table_64


interrupt_descriptor_table_64:
%rep IDT_NUM_ENTRIES
dw 0
dw CODE_SELECTOR
db 0
db PRESENT_BIT_SET | INTERRUPT_GATE_TYPE
dw 0
dd 0
dd 0
%endrep

IDT_64_SIZE_MINUS_1 equ $ - interrupt_descriptor_table_64 - 1


IDT_descriptor_64:
dw IDT_64_SIZE_MINUS_1
dq interrupt_descriptor_table_64
