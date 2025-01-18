#! /bin/sh

#
# Copyright (c) 2024 Logan Ryan McLintock
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

# Disk.
cylinders=20
heads=16
sectors=63

mbr_sector=1
loader_sectors=1
# kernel_sectors=120
asm=nasm
bytes_per_block=512

if grep -rEnI --exclude-dir=.git '.{80}'
then
    printf '%s: ERROR: Long lines\n' "$0" 1>&2
    exit 1
fi

find . -type d ! -path '*.git*'                -exec chmod 700 '{}' \;
find . -type f ! -path '*.git*' ! -name '*.sh' -exec chmod 600 '{}' \;
find . -type f ! -path '*.git*'   -name '*.sh' -exec chmod 700 '{}' \;


rm -f boot.img.lock

dd if=/dev/zero of=boot.img bs="$bytes_per_block" \
    count="$((cylinders * heads * sectors))"

"$asm" -f bin -o mbr.bin mbr.asm
"$asm" -f bin -o loader.bin loader.asm
"$asm" -f bin -o kernel.bin kernel.asm

dd if=mbr.bin of=boot.img conv=notrunc

dd if=loader.bin of=boot.img bs="$bytes_per_block" seek="$mbr_sector" \
    conv=notrunc

dd if=kernel.bin of=boot.img bs="$bytes_per_block" \
    seek="$((mbr_sector + loader_sectors))" conv=notrunc

qemu-system-x86_64 -cpu kvm64,pdpe1gb -m 1024 \
    -drive file=boot.img,index=0,media=disk,format=raw
