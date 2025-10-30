#! /bin/sh

#
# Copyright (c) 2024, 2025 Logan Ryan McLintock
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

# Build tools with options.


asm=nasm
# asm=yasm

cc=clang
# cc=gcc

ld=ld.lld
# ld=ld

objcopy=objcopy

lint=splint

indent=indent




asm_op() {
    "$asm" "$@"
}


cc_op() {
    # -DNDEBUG
    "$cc" \
        -ansi \
        -O0 \
        -Wall \
        -Wextra \
        -pedantic \
        -ffreestanding \
        -fno-stack-protector \
        -fno-builtin \
        -mcmodel=large \
        -mno-red-zone \
        -nostdlib \
        -mno-sse \
        -mno-avx \
        "$@"
}


ld_op() {
    if [ "$ld" = ld ]
    then
        set -- --no-warn-rwx-segments "$@"
    fi

    "$ld" -z noexecstack --nostdlib "$@"
}


objcopy_op() {
    "$objcopy" "$@"
}


lint_op() {
    "$lint" \
        -boolops \
        -predboolint \
        +charintliteral \
        -initallelements \
        -globuse \
        -mustfreeonly \
        -temptrans \
        -usedef \
        -compdef \
        -exportlocal \
        "$@"
}


indent_op() {
    "$indent" \
        -nut \
        -kr \
        -l79 \
        -bbo \
        -T uint8_t \
        -T uint16_t \
        -T uint32_t \
        -T uint64_t \
        "$@"
}
