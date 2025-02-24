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
global memcpy
global memmove
global memset
global memcmp


memcpy:
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
; Argument 2: rsi: Byte used to fill in the memory (passed as an integer).
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
; Argument 1: rdi: Memory A (mem_a).
; Argument 2: rsi: Memory B (mem_b).
; Argument 3: rdx: Size in bytes.
; Returns: rax:  0 = Equal.
;               -1 = First differing byte has mem_a before mem_b.
;                1 = First differing byte has mem_a after mem_b.

cld

; Swap destination and source, as cmpsb performs rsi - rdi,
; and need to do mem_a - mem_b.
mov r8, rdi
mov rdi, rsi
mov rsi, r8

mov rcx, rdx
repe cmpsb
jb .below
ja .above

; Equal.
xor rax, rax
ret

.below:
mov rax, -1
ret

.above:
mov rax, 1
ret
