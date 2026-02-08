#! /bin/sh

#
# Copyright (c) 2024, 2025 Logan Ryan McLintock. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
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
