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

#include "stddef.h"
#include "stdint.h"

#include "printf.h"


#define MEMORY_MAP_ENTRY_COUNT_ADDRESS 0x9000
#define DWORD_SIZE 4
#define MEMORY_MAP_ADDRESS (MEMORY_MAP_ENTRY_COUNT_ADDRESS + DWORD_SIZE)

#define KERNEL_SPACE_VIRTUAL_ADDRESS 0xffff800000000000

#define PDPT_SIZE 0x1000
#define BYTES_PER_PDPT_ENTRY 8
#define NUM_GIB_MAPPED (PDPT_SIZE / BYTES_PER_PDPT_ENTRY)
#define EXPONENT_1_GIB 30

#define MAX_MAPPED_VIRTUAL_ADDRESS_EXCLUSIVE (KERNEL_SPACE_VIRTUAL_ADDRESS  \
    + ((uint64_t) NUM_GIB_MAPPED << EXPONENT_1_GIB))


#define EXPONENT_2_MIB 21
#define PAGE_SIZE (1 << EXPONENT_2_MIB)

/*
 * Aligns up to the next page if not already aligned. The minus one is so
 * that an already aligned address will not move up to the next page.
 */
#define align_to_page(a) (((a) + PAGE_SIZE - 1) \
    >> EXPONENT_2_MIB << EXPONENT_2_MIB)

/* Truncates an address down to the start of its page. */
#define truncate_to_page(a) ((a) >> EXPONENT_2_MIB << EXPONENT_2_MIB)

/* Converts a physical address to a virtual address. */
#define phy_to_virt(a) ((a) + KERNEL_SPACE_VIRTUAL_ADDRESS)

/* Converts a virtual address to a physical address. */
#define virt_to_phy(a) ((a) - KERNEL_SPACE_VIRTUAL_ADDRESS)




extern char end;


struct address_range_descriptor {
    uint64_t address;
    uint64_t size;
    uint32_t type;
} __attribute__((packed));


struct physical_page {
    /*@null@ */ struct physical_page *next;
};

static struct physical_page head = { NULL };

static uint64_t num_free_pages = 0;


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


static void free_physical_page(struct physical_page
                               *start_physical_page)
{
    start_physical_page->next = head.next;
    head.next = start_physical_page;
    ++num_free_pages;
}


static /*@null@ */ void *allocate_physical_page(void)
{
    /* Returns the physical address of the start of the page. */
    struct physical_page *t;
    t = head.next;

    if (t == NULL)
        return NULL;

    /* Bypass. */
    head.next = t->next;

    return t;
}


static void free_virtual_range(uint64_t start_virtual_address,
                               uint64_t size)
{
    uint64_t end_virtual_address_exclusive, start_virtual_page,
        end_virtual_page_exclusive, i;

    end_virtual_address_exclusive = start_virtual_address + size;

    if (start_virtual_address < (uint64_t) & end)
        start_virtual_address = (uint64_t) & end;

    if (end_virtual_address_exclusive >
        MAX_MAPPED_VIRTUAL_ADDRESS_EXCLUSIVE)
        end_virtual_address_exclusive =
            MAX_MAPPED_VIRTUAL_ADDRESS_EXCLUSIVE;

    start_virtual_page = align_to_page(start_virtual_address);

    end_virtual_page_exclusive =
        truncate_to_page(end_virtual_address_exclusive);

    if (end_virtual_page_exclusive <= start_virtual_page)
        return;

    for (i = start_virtual_page; i < end_virtual_page_exclusive;
         i += PAGE_SIZE)
        free_physical_page((struct physical_page *) virt_to_phy(i));
}


int collect_free_memory(void)
{
    uint32_t i, num_entries;
    struct address_range_descriptor *p;

    num_entries = *(uint32_t *) MEMORY_MAP_ENTRY_COUNT_ADDRESS;
    p = (struct address_range_descriptor *) MEMORY_MAP_ADDRESS;

    for (i = 0; i < num_entries; ++i) {
        if (p->type == 1)
            free_virtual_range(phy_to_virt(p->address), p->size);

        ++p;
    }

    if (printf
        ("Number of free pages: %lu\n",
         (unsigned long) num_free_pages) == -1)
        return -1;

    return 0;
}
