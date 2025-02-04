/*
 * Copyright (c) 2025 Logan Ryan McLintock
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */


#include "stdint.h"

#include "printf.h"


#define MEMORY_MAP_ENTRY_COUNT_ADDRESS 0x9000
#define DWORD_SIZE 4
#define MEMORY_MAP_ADDRESS (MEMORY_MAP_ENTRY_COUNT_ADDRESS + DWORD_SIZE)


struct address_range_descriptor {
    uint64_t address;
    uint64_t size;
    uint32_t type;
} __attribute__((packed));


int print_memory_map(void)
{
    uint32_t i, num_entries;
    struct address_range_descriptor *p;

    num_entries = *(uint32_t *) MEMORY_MAP_ENTRY_COUNT_ADDRESS;
    p = (struct address_range_descriptor *) MEMORY_MAP_ADDRESS;

    for (i = 0; i < num_entries; ++i) {
        if (printf
            ("%lx : %lu : %lu\n", (unsigned long) p->address,
             (unsigned long) p->size, (unsigned long) p->type) == -1)
            return -1;

        ++p;
    }

    return 0;
}
