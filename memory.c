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

#define PML4_SIZE 0x1000
#define PDPT_SIZE 0x1000
#define BYTES_PER_PAGE_TABLE_ENTRY 8
#define NUM_GIB_MAPPED (PDPT_SIZE / BYTES_PER_PAGE_TABLE_ENTRY)
#define EXP_1_GIB 30

/* Refers to the GiB page size mapping created in the loader.asm file. */
#define MAX_MAPPED_VA_EXCL (KERNEL_SPACE_VA \
    + ((uint64_t) NUM_GIB_MAPPED << EXP_1_GIB))


#define EXP_2_MIB 21
#define PAGE_SIZE (1 << EXP_2_MIB)


#define PAGE_PRESENT    1
#define READ_AND_WRITE  1 << 1
#define USER_ACCESS     1 << 2
/* Page Size attribute. */
#define PS              1 << 7


#define FREE_PAGE_SIGNATURE 0xC0FFEECAFE0FC0DE


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
#define offset_component_va(v) ((v) & 0x1fffff)

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
static uint64_t max_pages = 0;
static uint64_t max_pa_excl = 0;


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
    *(uint64_t *) pa_to_va(start_page_pa + sizeof(uint64_t)) =
        FREE_PAGE_SIGNATURE;

    head = start_page_pa;
    ++num_free_pages;

    if (num_free_pages > max_pages)
        max_pages = num_free_pages;

    if (start_page_pa + PAGE_SIZE > max_pa_excl)
        max_pa_excl = start_page_pa + PAGE_SIZE;
}


static uint64_t allocate_page_pa(void)
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
            printf("ERROR: Physical memory: Page not aligned: %lx\n",
                   (unsigned long) h);
            return -1;
        }

        if (*(uint64_t *) pa_to_va(h + sizeof(uint64_t)) !=
            (uint64_t) FREE_PAGE_SIGNATURE) {
            printf("ERROR: Physical memory: Invalid signature\n");
            return -1;
        }

        h = *(uint64_t *) pa_to_va(h);  /* Next. */
        ++check_num_free_pages;
    }

    if (check_num_free_pages != num_free_pages) {
        printf("ERROR: Physical memory: Mismatch in number of free pages\n");
        printf("Checked: %lu, Reported: %lu\n",
               (unsigned long) check_num_free_pages,
               (unsigned long) num_free_pages);

        return -1;
    }

    printf("Memory check OK\n");
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


int report_physical_memory(void)
{
    if (printf
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
        if (p->type == 1)
            free_range_va(pa_to_va(p->pa), p->size);

        ++p;
    }

    if (report_physical_memory())
        return -1;

    if (printf
        ("Max physical memory exclusive: %lx\n",
         (unsigned long) max_pa_excl) == -1)
        return -1;

    return 0;
}


static int map_range(uint64_t pml4_pa, uint64_t start_va, uint64_t end_va_excl,
                     uint64_t start_pa, uint32_t attributes)
{
    /*
     * Page tables store physical addresses, but addresses need to be converted
     * to virtual addressed before they can be dereferenced, as the current
     * in-force paging must be used to access them.
     */

    uint64_t start_page_va, end_page_va_excl, v, x, p, pml4e_pa, pml4e_content,
        pdpte_pa, pdpte_content, pde_pa;

    /* Find superset page range -- a potentially wider range. */
    start_page_va = truncate_to_page(start_va);
    end_page_va_excl = align_to_page(end_va_excl);

    if (start_page_va >= end_page_va_excl)
        return -1;

    if (end_page_va_excl > MAX_MAPPED_VA_EXCL)
        return -1;

    x = start_pa;

    for (v = start_page_va; v < end_page_va_excl; v += PAGE_SIZE) {
        /* Level A. */
        pml4e_pa = pml4_pa + pml4_component_va(v) * BYTES_PER_PAGE_TABLE_ENTRY;
        pml4e_content = *(uint64_t *) pa_to_va(pml4e_pa);
        if (!(pml4e_content & PAGE_PRESENT)) {
            /* Allocate page for the Page-Directory-Pointer Table. */
            p = allocate_page_pa();
            if (p == 0)
                return -1;

            *(uint64_t *) pa_to_va(pml4e_pa) = p | attributes | PAGE_PRESENT;
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

            *(uint64_t *) pa_to_va(pdpte_pa) = p | attributes | PAGE_PRESENT;
        }
        pdpte_content = *(uint64_t *) pa_to_va(pdpte_pa);


        /* Level C. */
        pde_pa = clear_lower_bits(pdpte_content, 12)
            + dir_component_va(v) * BYTES_PER_PAGE_TABLE_ENTRY;

        /* Map the physical address. */
        *(uint64_t *) pa_to_va(pde_pa) = x | PS | attributes | PAGE_PRESENT;

        x += PAGE_SIZE;
    }

    return 0;
}


static int free_user_data_range(uint64_t pml4_pa, uint64_t start_va,
                                uint64_t end_va_excl)
{
    /*
     * Frees data pages in a range for user space.
     *
     * In user mode, PAGE_PRESENT can be used to keep track of allocations.
     *
     * However, in kernel mode, all of the virtual space (that is backed by
     * physical RAM) has PAGE_PRESENT set, so that the kernel can write to
     * most of RAM. For this reason, PAGE_PRESENT cannot be used as an
     * allocation indicator in kernel mode. The kernel needs to otherwise
     * remember which data pages it has requested to be allocated, and then
     * free them explicitly.
     */

    uint64_t start_page_va, end_page_va_excl, v, p, pml4e_pa, pml4e_content,
        pdpte_pa, pdpte_content, pde_pa, pde_content;

    /* Find superset page range -- a potentially wider range. */
    start_page_va = truncate_to_page(start_va);
    end_page_va_excl = align_to_page(end_va_excl);

    if (start_page_va >= end_page_va_excl)
        return -1;

    if (end_page_va_excl > MAX_MAPPED_VA_EXCL)
        return -1;

    for (v = start_page_va; v < end_page_va_excl; v += PAGE_SIZE) {
        /* Level A. */
        pml4e_pa = pml4_pa + pml4_component_va(v) * BYTES_PER_PAGE_TABLE_ENTRY;
        pml4e_content = *(uint64_t *) pa_to_va(pml4e_pa);
        if (pml4e_content & PAGE_PRESENT) {
            /* Level B. */
            pdpte_pa = clear_lower_bits(pml4e_content, 12)
                + dir_ptr_component_va(v) * BYTES_PER_PAGE_TABLE_ENTRY;
            pdpte_content = *(uint64_t *) pa_to_va(pdpte_pa);
            if (pdpte_content & PAGE_PRESENT) {
                /* Level C. */
                pde_pa = clear_lower_bits(pdpte_content, 12)
                    + dir_component_va(v) * BYTES_PER_PAGE_TABLE_ENTRY;

                pde_content = *(uint64_t *) pa_to_va(pde_pa);

                /* Free the physical data page. */
                if (pde_content & PAGE_PRESENT & USER_ACCESS) {
                    p = clear_lower_bits(pde_content, 21);
                    free_page_pa(p);
                    *(uint64_t *) pa_to_va(pde_pa) = 0;
                }
            }
        }
    }

    return 0;
}


void free_memory_space(uint64_t pml4_pa)
{
    /* Assumes that data pages have already been freed. */
    uint64_t i, j, pml4e_pa, pml4e_content, pdpt_pa, pdpte_pa, pdpte_content,
        pd_pa;

    for (i = 0; i < PML4_SIZE; i += BYTES_PER_PAGE_TABLE_ENTRY) {
        /* Level A. */
        pml4e_pa = pml4_pa + i;
        pml4e_content = *(uint64_t *) pa_to_va(pml4e_pa);
        if (pml4e_content & PAGE_PRESENT) {
            pdpt_pa = clear_lower_bits(pml4e_content, 12);
            for (j = 0; j < PDPT_SIZE; j += BYTES_PER_PAGE_TABLE_ENTRY) {
                /* Level B. */
                pdpte_pa = pdpt_pa + j;
                pdpte_content = *(uint64_t *) pa_to_va(pdpte_pa);
                if (pdpte_content & PAGE_PRESENT) {
                    pd_pa = clear_lower_bits(pdpte_content, 12);
                    free_page_pa(pd_pa);
                }
            }
            free_page_pa(pdpt_pa);
        }
    }
    free_page_pa(pml4_pa);
}


uint64_t create_user_virtual_memory_space(uint64_t exec_start_va,
                                          uint64_t exec_size)
{
    uint64_t pml4_pa, v, p, s, x, y;

    pml4_pa = allocate_page_pa();
    if (pml4_pa == 0)
        return 0;               /* Error. */


    v = exec_start_va;
    s = exec_size;
    y = USER_EXEC_START_VA;

    while (s) {
        p = allocate_page_pa();
        if (p == 0)
            goto clean_up;

        if (s <= PAGE_SIZE)
            x = s;
        else
            x = PAGE_SIZE;

        memcpy((void *) pa_to_va(p), (const void *) v, x);

        if (map_range
            (pml4_pa, y, y + PAGE_SIZE, p,
             (uint32_t) READ_AND_WRITE | USER_ACCESS)) {
            free_page_pa(p);
            goto clean_up;
        }

        v += x;
        s -= x;
        y += PAGE_SIZE;
    }

    return pml4_pa;

  clean_up:
    free_user_data_range(pml4_pa, USER_EXEC_START_VA, exec_size);
    free_memory_space(pml4_pa);

    return 0;                   /* Error. */
}


uint64_t create_kernel_virtual_memory_space(void)
{
    uint64_t pml4_pa;

    pml4_pa = allocate_page_pa();
    if (pml4_pa == 0)
        return 0;               /* Error. */

    if (map_range
        (pml4_pa, KERNEL_SPACE_VA, pa_to_va(max_pa_excl), 0,
         (uint32_t) READ_AND_WRITE)) {
        free_page_pa(pml4_pa);
        return 0;               /* Error. */
    }

    return pml4_pa;
}
