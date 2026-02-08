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

; Print in real mode for Toucanix.


%include "defs.inc"


[BITS 16]
[ORG PRINT_PA]

; Note that row index starts from zero, not one.
row: dd 0
col: dd 0

print_func:
; Argument 1: ds:si : Source string.
; Argument 2: bl : Colour.
push ax
push cx
push dx
push es
push di
push si

mov ax, VIDEO_SEGMENT
mov es, ax

.loop:
mov ax, [col]
cmp ax, SCREEN_WIDTH
jne .continue0
inc word [row]
mov word [col], 0
.continue0:
mov al, [ds:si]
test al, al
jz .done
cmp al, NL
jne .continue
inc word [row]
mov word [col], 0
inc si
jmp .loop
.continue:
mov ax, [row]
mov cx, BYTES_PER_LINE
mul cx
jc .done
push ax
mov ax, [col]
mov cx, BYTES_PER_SCREEN_CHAR
mul cx
jc .done
pop cx
add ax, cx
mov di, ax
mov al, [ds:si]
mov byte [es:di], al
inc di
mov byte [es:di], bl
inc word [col]
inc si
jmp .loop

.done:
pop si
pop di
pop es
pop dx
pop cx
pop ax

ret
