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

;
; The user interface to the system calls.
;
; I can do all things through Christ who strengthens me.
;                                               Philippians 4:13 NKJV
;


%include "../defs.inc"


section .text
global u_system_write
global u_sleep




u_system_write:
; Stack frame.
push rbp
mov rbp, rsp

; Push original args to the stack, in reverse order.
push rdx ; Arg 3: Size.
push rsi ; Arg 2: Pointer to data.
push rdi ; Arg 1: File descriptor.

; Send number of original args on the stack as the first new argument.
mov rdi, 3

; Send stack pointer as second new argument.
mov rsi, rsp

mov rax, SYS_CALL_WRITE
int SOFTWARE_INT

mov rsp, rbp
pop rbp
ret




u_sleep:
; Stack frame.
push rbp
mov rbp, rsp

; Push original args to the stack, in reverse order.
push rdi ; Arg 1: Seconds.

; Send number of original args on the stack as the first new argument.
mov rdi, 1

; Send stack pointer as second new argument.
mov rsi, rsp

mov rax, SYS_CALL_SLEEP
int SOFTWARE_INT

mov rsp, rbp
pop rbp
ret
