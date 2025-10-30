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

/*
 * Physical memory allocator.
 */


#include "defs.h"
#include "address.h"
#include "asm_lib.h"
#include "allocator.h"
#include "k_printf.h"


#define MEMORY_TYPE_USABLE 1
#define MEMORY_TYPE_RESERVED 2


#define FREE_PAGE_SIGNATURE 0xC0FFEECAFE0FC0DE


struct pa_range_descriptor {
    uint64_t pa;
    uint64_t size;
    uint32_t type;
} __attribute__((packed));


extern char end;

static uint64_t head = 0;
static uint64_t num_free_pages = 0;
static uint64_t max_pages = 0;
uint64_t max_pa_excl = 0;


int print_memory_map_pa(void)
{
    uint32_t i, num_entries;
    struct pa_range_descriptor *p;
    char *type_str;

    num_entries = *(uint32_t *) MEMORY_MAP_ENTRY_COUNT_VA;
    p = (struct pa_range_descriptor *) MEMORY_MAP_VA;

    for (i = 0; i < num_entries; ++i) {
        switch (p->type) {
        case MEMORY_TYPE_USABLE:
            type_str = "Usable";
            break;
        case MEMORY_TYPE_RESERVED:
            type_str = "Reserved";
            break;
        default:
            type_str = "Unknown";
            break;
        }
        if (k_printf("%lx => %lx: %s\n", (unsigned long) p->pa,
                     (unsigned long) p->pa + p->size, type_str) == -1)
            return -1;

        ++p;
    }

    return 0;
}


void free_page_pa(uint64_t start_page_pa)
{
    /*
     * Page starting at physical address zero cannot be used,
     * as it clashes with the indication of no more memory.
     */
    if (start_page_pa == 0)
        return;

    *(uint64_t *) pa_to_va(start_page_pa) = head;
    *(uint64_t *) pa_to_va(start_page_pa + sizeof(uint64_t)) =
        FREE_PAGE_SIGNATURE;

    head = start_page_pa;
    ++num_free_pages;

    if (num_free_pages > max_pages)
        max_pages = num_free_pages;

    if (start_page_pa + PAGE_SIZE > max_pa_excl)
        max_pa_excl = start_page_pa + PAGE_SIZE;
}


uint64_t allocate_page_pa(void)
{
    /* Returns the physical address of the start of the page. */
    uint64_t p;

    if (head == 0 || num_free_pages == 0)
        return 0;               /* No more physical memory. */

    p = head;

    /* Update head to the next page in the linked list. */
    head = *(uint64_t *) pa_to_va(head);

    /* Clear page. */
    memset((void *) pa_to_va(p), 0, (uint64_t) PAGE_SIZE);

    --num_free_pages;

    return p;
}


int check_physical_memory(void)
{
    uint64_t h, check_num_free_pages = 0;

    h = head;

    while (h != 0) {
        if (h % PAGE_SIZE) {
            (void) k_printf("ERROR: Physical memory: Page not aligned: %lx\n",
                            (unsigned long) h);
            return -1;
        }

        if (*(uint64_t *) pa_to_va(h + sizeof(uint64_t)) !=
            (uint64_t) FREE_PAGE_SIGNATURE) {
            (void) k_printf("ERROR: Physical memory: Invalid signature\n");
            return -1;
        }

        h = *(uint64_t *) pa_to_va(h);  /* Next. */
        ++check_num_free_pages;
    }

    if (check_num_free_pages != num_free_pages) {
        (void)
            k_printf
            ("ERROR: Physical memory: Mismatch in number of free pages\n");
        (void) k_printf("Checked: %lu, Reported: %lu\n",
                        (unsigned long) check_num_free_pages,
                        (unsigned long) num_free_pages);

        return -1;
    }

    (void) k_printf("Memory check OK\n");
    return 0;
}


static void free_range_va(uint64_t start_va, uint64_t size)
{
    /*
     * Range can be huge with the upper bound only limited by the GiB page size
     * mapping established in the loader.asm file.
     */
    uint64_t end_va_excl, start_page_va, end_page_va_excl, v;

    end_va_excl = start_va + size;

    /* Cannot use memory below the kernel code. */
    if (start_va < (uint64_t) &end)
        start_va = (uint64_t) &end;

    if (end_va_excl > MAX_MAPPED_VA_EXCL)
        end_va_excl = MAX_MAPPED_VA_EXCL;

    /* Find subset page range -- a potentially narrower range. */
    start_page_va = align_to_page(start_va);
    end_page_va_excl = truncate_to_page(end_va_excl);

    if (end_page_va_excl <= start_page_va)
        return;

    for (v = start_page_va; v < end_page_va_excl; v += PAGE_SIZE)
        free_page_pa(va_to_pa(v));
}


int report_physical_memory(void)
{
    if (k_printf
        ("Used physical pages: %lu/%lu\n", (unsigned long) num_free_pages,
         (unsigned long) max_pages)
        == -1)
        return -1;

    return 0;
}


int init_free_physical_memory(void)
{
    uint32_t i, num_entries;
    struct pa_range_descriptor *p;

    num_entries = *(uint32_t *) MEMORY_MAP_ENTRY_COUNT_VA;
    p = (struct pa_range_descriptor *) MEMORY_MAP_VA;

    for (i = 0; i < num_entries; ++i) {
        if (p->type == MEMORY_TYPE_USABLE)
            free_range_va(pa_to_va(p->pa), p->size);

        ++p;
    }

    if (report_physical_memory())
        return -1;

    if (k_printf
        ("Max physical memory exclusive: %lx\n",
         (unsigned long) max_pa_excl) == -1)
        return -1;

    return 0;
}
