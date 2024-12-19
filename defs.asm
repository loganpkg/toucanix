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

; Shared definitions for Toucanix.


%define MBR_ADDRESS 0x7c00
%define VIDEO_ADDRESS 0xb8000
video_segment equ VIDEO_ADDRESS / 16
loader_address equ MBR_ADDRESS + 512

; Colours (in hex).
%define BLACK 0
%define LIGHT_GREY 7
%define LIGHT_GREEN A
%define MAGENTA 5
%define YELLOW E

%define text_colour(bg, fg) 0x %+ bg %+ fg

; Colour combinations.
grey_on_black equ text_colour(BLACK, LIGHT_GREY)
green_on_black equ text_colour(BLACK, LIGHT_GREEN)
yellow_on_magenta equ text_colour(MAGENTA, YELLOW)
