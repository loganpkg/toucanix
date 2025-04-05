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


# Build script for Toucanix.

set -x
set -e
set -u

# Tools.
asm=nasm
# asm=yasm

cc=clang
# cc=gcc

ld=ld.lld
# ld=ld

objcopy=objcopy

lint=splint

indent=indent


# Options.
# -DNDEBUG
c_options='-ansi -O0 -Wall -Wextra -pedantic'
c_options="$c_options"' -ffreestanding -fno-stack-protector -fno-builtin'
c_options="$c_options"' -mcmodel=large -mno-red-zone'
c_options="$c_options"' -nostdlib -mno-sse -mno-avx'

ld_options='-z noexecstack --nostdlib -T linker_script.ld'

if [ "$ld" = ld ]
then
    ld_options='--no-warn-rwx-segments '"$ld_options"
fi


lint_options='-boolops -predboolint +charintliteral -initallelements -globuse'
lint_options="$lint_options"' -mustfreeonly -temptrans -usedef -compdef'


indent_options='-nut -kr -l79 -bbo'


# Export variables used in subshell.
export cc lint indent c_options lint_options indent_options


# Disk.
cylinders=20
heads=16
sectors=63

mbr_sector=1
print_sectors=1
loader_sectors=2
# kernel_sectors=120

bytes_per_block=512


# Fix permissions.
find . -type d ! -path '*.git*'                -exec chmod 700 '{}' \;
find . -type f ! -path '*.git*' ! -name '*.sh' -exec chmod 600 '{}' \;
find . -type f ! -path '*.git*'   -name '*.sh' -exec chmod 700 '{}' \;


# Clean up.
find . -type f ! -path '*.git*' \
    \( -name 'kernel'           \
    -o -name 'boot.img.lock'    \
    -o -name '*~'               \
    -o -name '*.o'              \
    -o -name '*.gch'            \
    -o -name '*.pch'            \
    \) -delete


if grep -rEnI --exclude-dir=.git '.{80}'
then
    printf '%s: ERROR: Long lines\n' "$0" 1>&2
    # exit 1
fi


shellcheck -e SC2086 build.sh


dd if=/dev/zero of=boot.img bs="$bytes_per_block" \
    count="$((cylinders * heads * sectors))"

"$asm" -f bin -o mbr.bin mbr.asm
"$asm" -f bin -o print.bin print.asm
"$asm" -f bin -o loader.bin loader.asm

"$asm" -f elf64 -o kernel_a.o kernel.asm
"$asm" -f elf64 -o interrupt_a.o interrupt.asm
"$asm" -f elf64 -o asm_lib_a.o asm_lib.asm
"$asm" -f elf64 -o memory_a.o memory.asm


find . -type f ! -path '*.git*' \( -name '*.c' -o -name '*.h' \) -exec sh -c '
    set -u
    fn="$1"
    if ! "$cc" -c $c_options "$fn"
    then
        exit 1
    fi
    "$indent" $indent_options "$fn"
    "$lint" $lint_options "$fn"
' sh '{}' \;


"$cc" -c $c_options -o kernel_c.o kernel.c
"$cc" -c $c_options -o interrupt_c.o interrupt.c
"$cc" -c $c_options -o printf_c.o printf.c
"$cc" -c $c_options -o screen_c.o screen.c
"$cc" -c $c_options -o memory_c.o memory.c
"$cc" -c $c_options -o process_c.o process.c


"$ld" $ld_options -o kernel \
kernel_a.o kernel_c.o interrupt_a.o interrupt_c.o asm_lib_a.o printf_c.o \
screen_c.o memory_a.o memory_c.o process_c.o

"$objcopy" -O binary kernel kernel.bin


dd if=mbr.bin of=boot.img conv=notrunc

dd if=print.bin of=boot.img bs="$bytes_per_block" seek="$mbr_sector" \
    conv=notrunc

dd if=loader.bin of=boot.img bs="$bytes_per_block" \
    seek="$((mbr_sector + print_sectors))" conv=notrunc

dd if=kernel.bin of=boot.img bs="$bytes_per_block" \
    seek="$((mbr_sector + print_sectors + loader_sectors))" conv=notrunc

qemu-system-x86_64 -cpu kvm64,pdpe1gb -m 1024 \
    -drive file=boot.img,index=0,media=disk,format=raw
