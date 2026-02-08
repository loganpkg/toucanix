;
; Copyright (c) 2025 Logan Ryan McLintock. All rights reserved.
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


%include "defs.inc"

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

global interrupt_return
global load_idt
global is_spurious_interrupt
global acknowledge_interrupt
global enter_process
global get_cr2
global switch_process


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

global system_software_interrupt
system_software_interrupt:
push NO_ERROR_CODE
push SOFTWARE_INT
jmp interrupt_common




interrupt_common:
push_all
mov rdi, rsp ; Prepare argument 1: Address of the interrupt stack frame.
call interrupt_handler




interrupt_return:
pop_all
; Remove error_code and vector_number from stack.
add rsp, 16
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




enter_process:
; Argument 1: rdi: Interrupt stack frame.
mov rsp, rdi
jmp interrupt_return




get_cr2:
; No arguments.
mov rax, cr2
ret




switch_process:
; Argument 1: rdi: Pointer to rsp_save of process that is exiting execution.
; Argument 2: rsi: rsp_save if of process entering execution.

; When a process is entered for the first time the exiting code below
; would have never been run. Hence, it is essential that the stack is prepared
; for the entry code below. This is done when the process is setup in the
; prepare_process function.

; Exiting process:
; Non-volatile registers (except for rsp).
push rbx
push rbp
push r12
push r13
push r14
push r15

mov [rdi], rsp  ; Backup rsp to the exiting process.


; Entering process:
mov rsp, rsi    ; Restore rsp from the entering process.

pop r15
pop r14
pop r13
pop r12
pop rbp
pop rbx

; Jumps to interrupt_return function.
ret
