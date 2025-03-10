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


#define MEMORY_MAP_ENTRY_COUNT_ADDRESS 0x9000

#define MEMORY_MAP_ENTRY_COUNT_VIRTUAL_ADDRESS (KERNEL_SPACE_VIRTUAL_ADDRESS \
    + MEMORY_MAP_ENTRY_COUNT_ADDRESS)

#define DWORD_SIZE 4

#define MEMORY_MAP_VIRTUAL_ADDRESS (MEMORY_MAP_ENTRY_COUNT_VIRTUAL_ADDRESS \
    + DWORD_SIZE)

#define PDPT_SIZE 0x1000
#define BYTES_PER_PAGE_TABLE_ENTRY 8
#define NUM_GIB_MAPPED (PDPT_SIZE / BYTES_PER_PAGE_TABLE_ENTRY)
#define EXPONENT_1_GIB 30

#define MAX_MAPPED_VIRTUAL_ADDRESS_EXCLUSIVE (KERNEL_SPACE_VIRTUAL_ADDRESS  \
    + ((uint64_t) NUM_GIB_MAPPED << EXPONENT_1_GIB))


#define EXPONENT_2_MIB 21
#define PAGE_SIZE (1 << EXPONENT_2_MIB)


#define PAGE_PRESENT    1
#define READ_AND_WRITE  1 << 1
#define USER_ACCESS     1 << 2
/* Page Size attribute. */
#define PS              1 << 7


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


/* Virtual (linear) address components for paging. */
#define pml4_component(v) ((v) >> 39 & 0x1ff)
#define dir_ptr_component(v) ((v) >> 30 & 0x1ff)
#define dir_component(v) ((v) >> 21 & 0x1ff)

/* Clears lower n bits. n is evaluated more than once. */
#define clear_lower_bits(p, n) ((p) >> (n) << (n))




extern char end;


struct address_range_descriptor {
    uint64_t address;
    uint64_t size;
    uint32_t type;
} __attribute__((packed));


static uint64_t head = 0;
static uint64_t num_free_pages = 0;




int print_memory_map(void)
{
    uint32_t i, num_entries;
    struct address_range_descriptor *p;

    num_entries = *(uint32_t *) MEMORY_MAP_ENTRY_COUNT_VIRTUAL_ADDRESS;
    p = (struct address_range_descriptor *) MEMORY_MAP_VIRTUAL_ADDRESS;

    for (i = 0; i < num_entries; ++i) {
        if (printf("%lx : %lu : %lu\n", (unsigned long) p->address,
                   (unsigned long) p->size, (unsigned long) p->type) == -1)
            return -1;

        ++p;
    }

    return 0;
}


static void free_physical_page(uint64_t start_physical_page)
{
    /*
     * Page starting at physical address zero cannot be used,
     * as it clashes with the indication of no more memory.
     */
    if (start_physical_page == 0)
        return;

    *(uint64_t *) phy_to_virt(start_physical_page) = head;
    head = start_physical_page;
    ++num_free_pages;
}


static uint64_t allocate_physical_page(void)
{
    /* Returns the physical address of the start of the page. */
    uint64_t head_next, head_next_next;

    if (head == 0 || num_free_pages == 0)
        return 0;               /* No more physical memory. */

    head_next = *(uint64_t *) phy_to_virt(head);

    if (head_next) {
        head_next_next = *(uint64_t *) phy_to_virt(head_next);

        /* Bypass. */
        head = head_next_next;
    } else {
        head = 0;
    }

    /* Clear page. */
    memset((void *) phy_to_virt(head_next), 0, (uint64_t) PAGE_SIZE);

    --num_free_pages;

    return head_next;
}


static void free_virtual_range(uint64_t start_virtual_address, uint64_t size)
{
    uint64_t end_virtual_address_exclusive, start_virtual_page,
        end_virtual_page_exclusive, i;

    end_virtual_address_exclusive = start_virtual_address + size;

    if (start_virtual_address < (uint64_t) & end)
        start_virtual_address = (uint64_t) & end;

    if (end_virtual_address_exclusive > MAX_MAPPED_VIRTUAL_ADDRESS_EXCLUSIVE)
        end_virtual_address_exclusive = MAX_MAPPED_VIRTUAL_ADDRESS_EXCLUSIVE;

    start_virtual_page = align_to_page(start_virtual_address);

    end_virtual_page_exclusive =
        truncate_to_page(end_virtual_address_exclusive);

    if (end_virtual_page_exclusive <= start_virtual_page)
        return;

    for (i = start_virtual_page; i < end_virtual_page_exclusive;
         i += PAGE_SIZE)
        free_physical_page(virt_to_phy(i));
}


int collect_free_memory(void)
{
    uint32_t i, num_entries;
    struct address_range_descriptor *p;

    num_entries = *(uint32_t *) MEMORY_MAP_ENTRY_COUNT_VIRTUAL_ADDRESS;
    p = (struct address_range_descriptor *) MEMORY_MAP_VIRTUAL_ADDRESS;

    for (i = 0; i < num_entries; ++i) {
        if (p->type == 1)
            free_virtual_range(phy_to_virt(p->address), p->size);

        ++p;
    }

    if (printf("Number of free pages: %lu\n", (unsigned long) num_free_pages)
        == -1)
        return -1;

    return 0;
}


static uint64_t create_pml4(void)
{
    /* Physical */
    return allocate_physical_page();
}


static int map_range(uint64_t pml4, uint64_t start_virtual_address,
                     uint64_t end_virtual_address_exclusive,
                     uint32_t attributes, int allocate_data)
{
    /*
     * Page tables store physical addresses, but addresses need to be converted
     * to virtual addressed before they can be dereferenced, as the current
     * in-force paging must be used to access them.
     */
    uint64_t s, e, v, p, pml4e, pml4e_content, pdpte, pdpte_content, pde,
        pde_content;

    s = truncate_to_page(start_virtual_address);
    e = align_to_page(end_virtual_address_exclusive);   /* Exclusive. */

    if (s >= e || e > MAX_MAPPED_VIRTUAL_ADDRESS_EXCLUSIVE)
        return -1;

    for (v = s; v < e; v += PAGE_SIZE) {
        /* Level A. */
        pml4e = pml4 + pml4_component(v) * BYTES_PER_PAGE_TABLE_ENTRY;
        pml4e_content = *(uint64_t *) phy_to_virt(pml4e);
        if (!(pml4e_content & PAGE_PRESENT)) {
            /* Allocate page for the Page-Directory-Pointer Table. */
            p = allocate_physical_page();
            if (p == 0)
                return -1;

            *(uint64_t *) phy_to_virt(pml4e) = p | attributes;
        }
        pml4e_content = *(uint64_t *) phy_to_virt(pml4e);


        /* Level B. */
        pdpte = clear_lower_bits(pml4e_content, 12)
            + dir_ptr_component(v) * BYTES_PER_PAGE_TABLE_ENTRY;
        pdpte_content = *(uint64_t *) phy_to_virt(pdpte);
        if (!(pdpte_content & PAGE_PRESENT)) {
            /* Allocate page for Page-Directory. */
            p = allocate_physical_page();
            if (p == 0)
                return -1;

            *(uint64_t *) phy_to_virt(pdpte) = p | attributes;
        }
        pdpte_content = *(uint64_t *) phy_to_virt(pdpte);


        /* Level C. */
        pde = clear_lower_bits(pdpte_content, 12)
            + dir_component(v) * BYTES_PER_PAGE_TABLE_ENTRY;

        if (allocate_data) {
            /* Allocate new physical memory to back the new virtual address. */
            pde_content = *(uint64_t *) phy_to_virt(pde);
            if (!(pde_content & PAGE_PRESENT)) {
                /* Allocate page for storage. */
                p = allocate_physical_page();
                if (p == 0)
                    return -1;

                *(uint64_t *) phy_to_virt(pde) = p | PS | attributes;
            }
            pde_content = *(uint64_t *) phy_to_virt(pde);
        } else {
            /*
             * Use the physical address that is currently backing the in-force
             * virtual address.
             */
            *(uint64_t *) phy_to_virt(pde) = virt_to_phy(v) | PS | attributes;
        }
    }
    return 0;
}

int init_kernel_virtual_memory_space(void)
{
    uint64_t pml4;

    pml4 = create_pml4();
    if (pml4 == 0)
        return -1;

    if (map_range
        (pml4, KERNEL_VIRTUAL_ADDRESS, (uint64_t) & end,
         (uint32_t) (READ_AND_WRITE | PAGE_PRESENT), 0))
        return -1;

    if (map_range
        (pml4, VIDEO_VIRTUAL_ADDRESS,
         (uint64_t) (VIDEO_VIRTUAL_ADDRESS + PAGE_SIZE),
         (uint32_t) (READ_AND_WRITE | PAGE_PRESENT), 0))
        return -1;

    switch_pml4(pml4);

    return 0;
}
