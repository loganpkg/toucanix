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


PIC_MASTER_COMMAND  equ 0x20

; Operation Control Word 3.
OCW_3_FIXED_BIT     equ 1 << 3
READ_REGISTER       equ 1 << 1
IN_SERVICE_REGISTER equ 1

IRQ_7_MASK          equ 1 << 7

; Non-Specific.
END_OF_INTERRUPT equ 1 << 5

NO_ERROR_CODE equ 0


section .text
extern interrupt_handler

global load_idt
global is_spurious_interrupt
global acknowledge_interrupt


%macro push_all 0
push rax
push rbx
push rcx
push rdx
push rsi
push rdi
push rbp
; push rsp
push r8
push r9
push r10
push r11
push r12
push r13
push r14
push r15
%endmacro

%macro pop_all 0
pop r15
pop r14
pop r13
pop r12
pop r11
pop r10
pop r9
pop r8
; pop rsp
pop rbp
pop rdi
pop rsi
pop rdx
pop rcx
pop rbx
pop rax
%endmacro


%macro make_vector 1
global vector_%1
vector_%1:
push NO_ERROR_CODE
push %1
jmp interrupt_common
%endmacro

%macro make_vector_has_error_code 1
global vector_%1
vector_%1:
push %1
jmp interrupt_common
%endmacro


make_vector 0
make_vector 1
make_vector 2
make_vector 3
make_vector 4
make_vector 5
make_vector 6
make_vector 7
make_vector_has_error_code 8
; Vector 9 is reserved.
make_vector_has_error_code 10
make_vector_has_error_code 11
make_vector_has_error_code 12
make_vector_has_error_code 13
make_vector_has_error_code 14
; Vector 15 is reserved.
make_vector 16
make_vector_has_error_code 17
make_vector 18
make_vector 19

make_vector 32
make_vector 39




interrupt_common:
push_all
mov rdi, rsp ; Prepare argument 1: Address of the interrupt stack frame.
call interrupt_handler
; Remove error_code and vector_number from stack.
add rsp, 16
pop_all
iretq


load_idt:
; Argument 1: rdi: Address of (pointer to) the IDT Descriptor.
lidt [rdi]
ret


is_spurious_interrupt:
; No arguments.
; Check for spurious interrupt.
mov al, OCW_3_FIXED_BIT | READ_REGISTER | IN_SERVICE_REGISTER
out PIC_MASTER_COMMAND, al
in al, PIC_MASTER_COMMAND
test al, IRQ_7_MASK
jz .ok
mov rax, 1
jmp .done
.ok:
mov rax, 0
.done:
ret


acknowledge_interrupt:
; No arguments.
mov al, END_OF_INTERRUPT
out PIC_MASTER_COMMAND, al
ret
