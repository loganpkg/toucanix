#! /bin/sh

#
# Copyright (c) 2024-2026 Logan Ryan McLintock. All rights reserved.
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


# Build script for Toucanix.

set -x
set -e
set -u


tmp=$(mktemp)

. ./tools.sh


get_var() {
    code=''
    # shellcheck disable=SC2016
    code=$(grep -E "$1 +equ" defs.inc | sed -E 's~ +equ +(.+)~="$((\1))"~')
    if [ -z "$code" ]
    then
        printf '%s: ERROR: Cannot find definition for variable: %s\n' \
            "$0" "$1" 1>&2

        exit 1
    fi

    printf '%s\n' "$code"
    eval "$code"
}


clean_up() {
find . -type f ! -path '*.git*' \
    \( -name 'kernel'           \
    -o -name 'boot.img.lock'    \
    -o -name 'a.out'            \
    -o -name '*~'               \
    -o -name '*.o'              \
    -o -name '*.a'              \
    -o -name '*.gch'            \
    -o -name '*.pch'            \
    \) -delete
}


get_var CYLINDERS
get_var HEADS
get_var SECTORS

get_var MBR_SECTOR
get_var PRINT_SECTORS
get_var LOADER_SECTORS
get_var KERNEL_SECTORS
get_var USER_A_SECTORS
get_var USER_B_SECTORS

get_var BYTES_PER_SECTOR

get_var PRINT_START_SECTOR
get_var LOADER_START_SECTOR
get_var KERNEL_START_SECTOR
get_var USER_A_START_SECTOR
get_var USER_B_START_SECTOR




# Generate files.
sed -E \
    -e 's~;>~/*~g' \
    -e 's~;<~ */~g' \
    -e 's~;;~ *~g' \
    -e 's~;\+ ~~g' \
    -e 's~; (.+)~/* \1 */~' \
    -e 's~(.*[^ ])( *) equ( +)(.*)~#define \1\2\3\4~' \
    defs.inc \
    | sed -E \
        -e 's~(\+ \()(NUM_GIB_MAPPED << EXP_1_GIB)~\\\n    \1(uint64_t) \2~' \
        -e 's~(#define +[^ ]+ +)(.*[^A-Za-z0-9_ ].*)~\1(\2)~' \
        > defs.h


sed -E \
    -e 's~k_printf~printf~g' \
    -e 's~#include "screen\.h"~~' \
    -e 's~(defs\.h)~../\1~' \
    -e 's~/\*\+ ~~g' \
    -e 's~ \+ *\*/~~g' \
    -e 's~^.*/\*-\*/$~~' \
    -e 's~^.*write_to_screen.*$~~' \
    k_printf.c > user_lib/printf.c

sed -E \
    -e 's~k_printf~printf~g' \
    -e 's~K_PRINTF~PRINTF~g' \
    k_printf.h > user_lib/printf.h

sed -E \
    -e 's~^.*/\*-\*/$~~' \
    -e 's~/\*\+ ~~g' \
    -e 's~ \+\*/~~g' \
    linker_script.ld > user_lib/u_linker_script.ld




# Fix permissions.
find . -type d ! -path '*.git*'                -exec chmod 700 '{}' \;
find . -type f ! -path '*.git*' ! -name '*.sh' -exec chmod 600 '{}' \;
find . -type f ! -path '*.git*'   -name '*.sh' -exec chmod 700 '{}' \;


clean_up


if grep -rEnI --exclude-dir=.git '.{80}'
then
    printf '%s: ERROR: Long lines\n' "$0" 1>&2
    exit 1
fi


# Check for duplicated definitions.
find . -type f ! -path '*.git/*' ! -name 'defs.h' ! -name 'assert.h' \
    \( -name '*.inc' -o -name '*.asm' -o -name '*.c' -o -name '*.h' \) \
        -exec grep -E '#define | equ ' '{}' \; \
            | sed -E 's/#define //' \
            | tr '(' ' ' \
            | sed -E 's/^ +//' \
            | cut -d ' ' -f 1 \
            | sort \
            | uniq --count \
            | grep -v -E ' 1 ' > "$tmp" || true


if [ -s "$tmp" ]
then
    printf '%s: ERROR: Duplicated definitions' "$0" 1>&2
    printf '%s\n' "$tmp" 1>&2
    cat "$tmp" 1>&2
    exit 1
fi


shellcheck build.sh tools.sh


cc -ansi -pedantic -Wall -Wextra show_defs.c
./a.out | column -s ':' -t -R 2


dd if=/dev/zero of=boot.img bs="$BYTES_PER_SECTOR" \
    count="$((CYLINDERS * HEADS * SECTORS))"

asm_op -f bin -o mbr.bin mbr.asm
asm_op -f bin -o print.bin print.asm
asm_op -f bin -o loader.bin loader.asm

asm_op -f elf64 -o kernel_a.o kernel.asm
asm_op -f elf64 -o interrupt_a.o interrupt.asm
asm_op -f elf64 -o asm_lib_a.o asm_lib.asm
asm_op -f elf64 -o paging_a.o paging.asm

cd user_lib || exit 1
asm_op -f elf64 -o u_system_call_a.o u_system_call.asm
asm_op -f elf64 -o _start_a.o _start.asm
cd .. || exit 1

find . -type f ! -path '*.git*' \( -name '*.c' -o -name '*.h' \) -exec sh -c '
    set -u
    . ./tools.sh
    fn="$1"
    if ! cc_op -c "$fn"
    then
        exit 1
    fi
    indent_op "$fn"
    lint_op "$fn"
' sh '{}' \;


cc_op -c -o kernel_c.o kernel.c
cc_op -c -o interrupt_c.o interrupt.c
cc_op -c -o k_printf_c.o k_printf.c
cc_op -c -o screen_c.o screen.c
cc_op -c -o allocator_c.o allocator.c
cc_op -c -o paging_c.o paging.c
cc_op -c -o process_c.o process.c
cc_op -c -o system_call_c.o system_call.c
cc_op -c -o user_lib/printf_c.o user_lib/printf.c
cc_op -c -o user_app_a/hello_world_c.o user_app_a/hello_world.c
cc_op -c -o user_app_b/hello_world_c.o user_app_b/hello_world.c


# Create user lib archive.
ar rsc user_lib/user_lib.a user_lib/u_system_call_a.o user_lib/printf_c.o


ld_op -T linker_script.ld -o kernel \
    kernel_a.o kernel_c.o interrupt_a.o interrupt_c.o asm_lib_a.o \
    k_printf_c.o screen_c.o allocator_c.o paging_a.o paging_c.o process_c.o \
    system_call_c.o


ld_op -T user_lib/u_linker_script.ld -o user_app_a/user_a \
    user_app_a/hello_world_c.o user_lib/_start_a.o user_lib/user_lib.a

ld_op -T user_lib/u_linker_script.ld -o user_app_b/user_b \
    user_app_b/hello_world_c.o user_lib/_start_a.o user_lib/user_lib.a


objcopy_op -O binary kernel kernel.bin
objcopy_op -O binary user_app_a/user_a user_app_a/user_a.bin
objcopy_op -O binary user_app_b/user_b user_app_b/user_b.bin


dd if=mbr.bin of=boot.img conv=notrunc

dd if=print.bin of=boot.img bs="$BYTES_PER_SECTOR" \
    seek="$PRINT_START_SECTOR" conv=notrunc

dd if=loader.bin of=boot.img bs="$BYTES_PER_SECTOR" \
    seek="$LOADER_START_SECTOR" conv=notrunc

dd if=kernel.bin of=boot.img bs="$BYTES_PER_SECTOR" \
    seek="$KERNEL_START_SECTOR" conv=notrunc

dd if=user_app_a/user_a.bin of=boot.img bs="$BYTES_PER_SECTOR" \
    seek="$USER_A_START_SECTOR" conv=notrunc

dd if=user_app_b/user_b.bin of=boot.img bs="$BYTES_PER_SECTOR" \
    seek="$USER_B_START_SECTOR" conv=notrunc


qemu-system-x86_64 -cpu kvm64,pdpe1gb -m 1024 \
    -drive file=boot.img,index=0,media=disk,format=raw

clean_up
