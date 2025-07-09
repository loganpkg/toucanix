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

;
; The user interface to the system calls.
;
; I can do all things through Christ who strengthens me.
;                                               Philippians 4:13 NKJV
;


%include "defs.inc"


section .text
global u_system_write

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
