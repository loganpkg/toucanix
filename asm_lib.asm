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
