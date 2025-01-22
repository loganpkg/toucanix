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
; IRQ = Interrupt ReQuest.
PIC_MASTER_COMMAND  equ 0x20
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

; Operation Control Word 3.
OCW_3_FIXED_BIT     equ 1 << 3
READ_REGISTER       equ 1 << 1
IN_SERVICE_REGISTER equ 1

IRQ_7_MASK          equ 1 << 7

; Non-Specific.
END_OF_INTERRUPT equ 1 << 5


; GDT.
USER_CODE_SEGMENT_INDEX equ 2
USER_DATA_SEGMENT_INDEX equ 3
TSS_INDEX equ 4

USER_CODE_SELECTOR equ USER_CODE_SEGMENT_INDEX << 3 | USER_RING
USER_DATA_SELECTOR equ USER_DATA_SEGMENT_INDEX << 3 | USER_RING
TSS_SELECTOR equ TSS_INDEX << 3

RFLAGS_INTERRUPT_ENABLE equ 1 << 9
RFLAGS_RESERVED_BIT_1 equ 1 << 1

CPL_MASK equ 3

KERNEL_STACK_ADDRESS equ 0x150000
TSS_SIZE equ 104

TSS_AVAILABLE equ 9


[BITS 64]
[ORG KERNEL_NEW_ADDRESS]

; Complete TSS entry in GDT.
mov rax, task_state_segment
mov word [tss_base_a_word], ax
shr rax, 16
mov byte [tss_base_b_byte], al
shr rax, 8
mov byte [tss_base_c_byte], al
shr rax, 8
mov dword [tss_base_d_dword], eax



mov rax, divide_by_zero
mov rdi, interrupt_descriptor_table
call set_interrupt_service_routine

mov rax, interval_timer
mov rdi, interrupt_descriptor_table + IRQ_0_MAP * IDT_ENTRY_SIZE
call set_interrupt_service_routine

mov rax, interrupt_service_routine_7
mov rdi, interrupt_descriptor_table + IRQ_7_MAP * IDT_ENTRY_SIZE
call set_interrupt_service_routine


lgdt [GDT_descriptor]
lidt [IDT_descriptor]
mov ax, TSS_SELECTOR
ltr ax


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


; Enter user mode.
push USER_DATA_SELECTOR
push MBR_ADDRESS
push RFLAGS_INTERRUPT_ENABLE | RFLAGS_RESERVED_BIT_1
push USER_CODE_SELECTOR
push user_start
iretq


user_start:

; Check CPL.
mov ax, cs
and ax, CPL_MASK
cmp ax, USER_RING
jne done

mov byte [VIDEO_ADDRESS], '3'
mov byte [VIDEO_ADDRESS + 1], GREEN_ON_BLACK

jmp user_start

done:
jmp done


set_interrupt_service_routine:
; Arguments:
; rax: Address of the interrupt service routine.
; rdi: Address of the entry in the interrupt descriptor table (IDT).

push rax

mov [rdi], ax       ; Offset: Lower two bytes.
shr rax, 16
mov [rdi + OFFSET_16_TO_31], ax
shr rax, 16
mov [rdi + OFFSET_32_TO_63], eax

pop rax
ret


divide_by_zero:
%include "push_all.asm"
mov byte [VIDEO_ADDRESS], 'Z'
mov byte [VIDEO_ADDRESS + 1], RED_ON_BLACK
jmp done
%include "pop_all.asm"
iretq


interval_timer:
%include "push_all.asm"
mov byte [VIDEO_ADDRESS + 2], 'I'
mov byte [VIDEO_ADDRESS + 2 + 1], GREEN_ON_BLACK

mov al, END_OF_INTERRUPT
out PIC_MASTER_COMMAND, al

%include "pop_all.asm"
iretq


interrupt_service_routine_7:
%include "push_all.asm"

; Check for spurious interrupt.
mov al, OCW_3_FIXED_BIT | READ_REGISTER | IN_SERVICE_REGISTER
out PIC_MASTER_COMMAND, al
in al, PIC_MASTER_COMMAND

test al, IRQ_7_MASK
jz .done

mov al, END_OF_INTERRUPT
out PIC_MASTER_COMMAND, al

.done:

%include "pop_all.asm"
iretq




; Data.

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

; TSS entry. Double the usual GDT entry size.
dw TSS_SIZE - 1 & 0xffff
tss_base_a_word:
dw 0
tss_base_b_byte:
db 0
db PRESENT_BIT_SET | TSS_AVAILABLE
db TSS_SIZE - 1 >> 16
tss_base_c_byte:
db 0
tss_base_d_dword:
dd 0
dd 0

GDT_SIZE equ $ - global_descriptor_table


GDT_descriptor:
dw GDT_SIZE - 1
dq global_descriptor_table


interrupt_descriptor_table:
%rep IDT_NUM_ENTRIES
dw 0
dw CODE_SELECTOR
db 0
db PRESENT_BIT_SET | INTERRUPT_GATE_TYPE
dw 0
dd 0
dd 0
%endrep

IDT_SIZE equ $ - interrupt_descriptor_table


IDT_descriptor:
dw IDT_SIZE - 1
dq interrupt_descriptor_table


task_state_segment:
dd 0
dq KERNEL_STACK_ADDRESS
times TSS_SIZE - ($ - task_state_segment) - BYTES_PER_DOUBLE_WORD db 0
dd TSS_SIZE
