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

; Print in real mode for Toucanix.


%include "defs.asm"


SCREEN_WIDTH equ 80
SCREEN_HEIGHT equ 25
BYTES_PER_SCREEN_CHAR equ 2
BYTES_PER_LINE equ SCREEN_WIDTH * BYTES_PER_SCREEN_CHAR


[BITS 16]
[ORG PRINT_ADDRESS]

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
