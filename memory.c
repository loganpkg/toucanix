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


#include "address.h"
#include "asm_lib.h"
#include "assert.h"
#include "memory.h"
#include "printf.h"


#define MEMORY_MAP_ENTRY_COUNT_PA 0x9000
#define MEMORY_MAP_ENTRY_COUNT_VA (KERNEL_SPACE_VA + MEMORY_MAP_ENTRY_COUNT_PA)
#define DWORD_SIZE 4
#define MEMORY_MAP_VA (MEMORY_MAP_ENTRY_COUNT_VA + DWORD_SIZE)

#define PDPT_SIZE 0x1000
#define BYTES_PER_PAGE_TABLE_ENTRY 8
#define NUM_GIB_MAPPED (PDPT_SIZE / BYTES_PER_PAGE_TABLE_ENTRY)
#define EXP_1_GIB 30

#define MAX_MAPPED_VA_EXCL (KERNEL_SPACE_VA \
    + ((uint64_t) NUM_GIB_MAPPED << EXP_1_GIB))


#define EXP_2_MIB 21
#define PAGE_SIZE (1 << EXP_2_MIB)


#define PAGE_PRESENT    1
#define READ_AND_WRITE  1 << 1
#define USER_ACCESS     1 << 2
/* Page Size attribute. */
#define PS              1 << 7


/*
 * Aligns up to the next page if not already aligned. The minus one is so
 * that an already aligned address will not move up to the next page.
 */
#define align_to_page(a) (((a) + PAGE_SIZE - 1) >> EXP_2_MIB << EXP_2_MIB)

/* Truncates an address down to the start of its page. */
#define truncate_to_page(a) ((a) >> EXP_2_MIB << EXP_2_MIB)


/* Virtual (linear) address components for paging. */
#define pml4_component_va(v) ((v) >> 39 & 0x1ff)
#define dir_ptr_component_va(v) ((v) >> 30 & 0x1ff)
#define dir_component_va(v) ((v) >> 21 & 0x1ff)

/* Clears lower n bits. n is evaluated more than once. */
#define clear_lower_bits(p, n) ((p) >> (n) << (n))




extern char end;

struct pa_range_descriptor {
    uint64_t pa;
    uint64_t size;
    uint32_t type;
} __attribute__((packed));


static uint64_t head = 0;
static uint64_t num_free_pages = 0;




int print_memory_map_pa(void)
{
    uint32_t i, num_entries;
    struct pa_range_descriptor *p;

    num_entries = *(uint32_t *) MEMORY_MAP_ENTRY_COUNT_VA;
    p = (struct pa_range_descriptor *) MEMORY_MAP_VA;

    for (i = 0; i < num_entries; ++i) {
        if (printf("%lx : %lu : %lu\n", (unsigned long) p->pa,
                   (unsigned long) p->size, (unsigned long) p->type) == -1)
            return -1;

        ++p;
    }

    return 0;
}


static void free_page_pa(uint64_t start_page_pa)
{
    /*
     * Page starting at physical address zero cannot be used,
     * as it clashes with the indication of no more memory.
     */
    if (start_page_pa == 0)
        return;

    *(uint64_t *) pa_to_va(start_page_pa) = head;
    head = start_page_pa;
    ++num_free_pages;
}


static uint64_t allocate_page_pa(void)
{
    /* Returns the physical address of the start of the page. */
    uint64_t head_next, head_next_next;

    if (head == 0 || num_free_pages == 0)
        return 0;               /* No more physical memory. */

    head_next = *(uint64_t *) pa_to_va(head);

    if (head_next) {
        head_next_next = *(uint64_t *) pa_to_va(head_next);

        /* Bypass. */
        head = head_next_next;
    } else {
        head = 0;
    }

    /* Clear page. */
    memset((void *) pa_to_va(head_next), 0, (uint64_t) PAGE_SIZE);

    --num_free_pages;

    return head_next;
}


static void free_range_va(uint64_t start_va, uint64_t size)
{
    uint64_t end_va_excl, start_page_va, end_page_va_excl, v;

    end_va_excl = start_va + size;

    if (start_va < (uint64_t) & end)
        start_va = (uint64_t) & end;

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


int collect_free_memory(void)
{
    uint32_t i, num_entries;
    struct pa_range_descriptor *p;

    num_entries = *(uint32_t *) MEMORY_MAP_ENTRY_COUNT_VA;
    p = (struct pa_range_descriptor *) MEMORY_MAP_VA;

    for (i = 0; i < num_entries; ++i) {
        if (p->type == 1)
            free_range_va(pa_to_va(p->pa), p->size);

        ++p;
    }

    if (printf("Number of free pages: %lu\n", (unsigned long) num_free_pages)
        == -1)
        return -1;

    return 0;
}


static uint64_t create_pml4_pa(void)
{
    /* Physical */
    return allocate_page_pa();
}


static int map_range(uint64_t pml4_pa, uint64_t start_va, uint64_t end_va_excl,
                     uint32_t attributes, int allocate_data)
{
    /*
     * Page tables store physical addresses, but addresses need to be converted
     * to virtual addressed before they can be dereferenced, as the current
     * in-force paging must be used to access them.
     */
    uint64_t start_page_va, end_page_va_excl, v, p, pml4e_pa, pml4e_content,
        pdpte_pa, pdpte_content, pde_pa, pde_content;

    /* Find superset page range -- a potentially wider range. */
    start_page_va = truncate_to_page(start_va);
    end_page_va_excl = align_to_page(end_va_excl);

    if (start_page_va >= end_page_va_excl
        || end_page_va_excl > MAX_MAPPED_VA_EXCL)
        return -1;

    for (v = start_page_va; v < end_page_va_excl; v += PAGE_SIZE) {
        /* Level A. */
        pml4e_pa = pml4_pa + pml4_component_va(v) * BYTES_PER_PAGE_TABLE_ENTRY;
        pml4e_content = *(uint64_t *) pa_to_va(pml4e_pa);
        if (!(pml4e_content & PAGE_PRESENT)) {
            /* Allocate page for the Page-Directory-Pointer Table. */
            p = allocate_page_pa();
            if (p == 0)
                return -1;

            *(uint64_t *) pa_to_va(pml4e_pa) = p | attributes;
        }
        pml4e_content = *(uint64_t *) pa_to_va(pml4e_pa);


        /* Level B. */
        pdpte_pa = clear_lower_bits(pml4e_content, 12)
            + dir_ptr_component_va(v) * BYTES_PER_PAGE_TABLE_ENTRY;
        pdpte_content = *(uint64_t *) pa_to_va(pdpte_pa);
        if (!(pdpte_content & PAGE_PRESENT)) {
            /* Allocate page for Page-Directory. */
            p = allocate_page_pa();
            if (p == 0)
                return -1;

            *(uint64_t *) pa_to_va(pdpte_pa) = p | attributes;
        }
        pdpte_content = *(uint64_t *) pa_to_va(pdpte_pa);


        /* Level C. */
        pde_pa = clear_lower_bits(pdpte_content, 12)
            + dir_component_va(v) * BYTES_PER_PAGE_TABLE_ENTRY;

        if (allocate_data) {
            /* Allocate new physical memory to back the new virtual address. */
            pde_content = *(uint64_t *) pa_to_va(pde_pa);
            if (!(pde_content & PAGE_PRESENT)) {
                /* Allocate page for storage. */
                p = allocate_page_pa();
                if (p == 0)
                    return -1;

                *(uint64_t *) pa_to_va(pde_pa) = p | PS | attributes;
            }
            pde_content = *(uint64_t *) pa_to_va(pde_pa);
        } else {
            /*
             * Use the physical address that is currently backing the in-force
             * virtual address.
             */
            *(uint64_t *) pa_to_va(pde_pa) = va_to_pa(v) | PS | attributes;
        }
    }
    return 0;
}

int init_kernel_virtual_memory_space(void)
{
    uint64_t pml4_pa;

    pml4_pa = create_pml4_pa();
    if (pml4_pa == 0)
        return -1;

    if (map_range
        (pml4_pa, KERNEL_VA, (uint64_t) & end,
         (uint32_t) (READ_AND_WRITE | PAGE_PRESENT), 0))
        return -1;

    if (map_range
        (pml4_pa, VIDEO_VA, (uint64_t) (VIDEO_VA + PAGE_SIZE),
         (uint32_t) (READ_AND_WRITE | PAGE_PRESENT), 0))
        return -1;

    switch_pml4_pa(pml4_pa);

    return 0;
}
