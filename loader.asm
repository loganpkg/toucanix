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

; Loader for Toucanix.

%include "defs.asm"

%define SYSTEM_MEMORY_MAP_ADDRESS 0x9000
%define BIOS_SYSTEM_SERVICES 0x15
%define SYSTEM_ADDRESS_MAP_FUNCTION_CODE 0xe820
; SMAP. PAMS in little-endian.
%define SYSTEM_MAP_SIGNATURE 0x534D4150
%define ADDRESS_RANGE_DESCRIPTOR_SIZE 20



[BITS 16]
[ORG loader_address]


; Save the system memory map.
mov edi, SYSTEM_MEMORY_MAP_ADDRESS
xor ebx, ebx ; Continuation value
loop:
mov ecx, ADDRESS_RANGE_DESCRIPTOR_SIZE
mov edx, SYSTEM_MAP_SIGNATURE
mov eax, SYSTEM_ADDRESS_MAP_FUNCTION_CODE
int BIOS_SYSTEM_SERVICES
jc error
test ebx, ebx
jz ok
add edi, ADDRESS_RANGE_DESCRIPTOR_SIZE
jmp loop

error:
mov ax, video_segment
mov es, ax
xor di, di
mov byte [es:di], 'e'
mov byte [es:di + 1], yellow_on_magenta
jmp done

ok:
mov ax, video_segment
mov es, ax
xor di, di
mov byte [es:di], 'K'
mov byte [es:di + 1], green_on_black

done:
hlt
jmp done
