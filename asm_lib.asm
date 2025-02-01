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


section .text
global memmove
global memset
global memcmp


memmove:
; Argument 1: rdi: Destination address.
; Argument 2: rsi: Source address.
; Argument 3: rdx: Size in bytes.
; Returns: rax: (Original) Destination address.
cld ; Default is forwards.
mov rcx, rdx ; Number of bytes to copy.
mov rax, rdi ; Return value.

; if (dest <= source || dest >= (source + size))
; then copy_forwards.

mov r10, rsi
add r10, rdx
cmp rdi, rsi
jbe .copy
cmp rdi, r10
jae .copy

; Copy backwards.
std
; Adjust destination and source to the end of the region.
; hello
; 01234
;     ^
;     |
;     size - 1
;
sub rdx, 1
add rdi, rdx
add rsi, rdx

.copy:
rep movsb
cld
ret


memset:
; Argument 1: rdi: Destination address.
; Argument 2: rsi: int cast as an unsigned char (used to fill in the memory).
; Argument 3: rdx: Size in bytes.
; Returns: rax: (Original) Destination address.
cld
mov r10, rdi
mov rax, rsi
mov rcx, rdx
rep stosb
mov rax, r10
ret


memcmp:
; Argument 1: rdi: Memory 1.
; Argument 2: rsi: Memory 2.
; Argument 3: rdx: Size in bytes.
; Returns: rax: 0 = Equal. 1 = Not equal.
cld
xor rax, rax
mov rcx, rdx
repe cmpsb
setne al
ret
