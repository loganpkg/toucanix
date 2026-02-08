/*
 * Copyright (c) 2025, 2026 Logan Ryan McLintock. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#include "defs.h"
#include "address.h"
#include "asm_lib.h"
#include "allocator.h"
#include "paging.h"
#include "k_printf.h"


/* Virtual (linear) address components for paging. */
#define pml4_component_va(v) ((v) >> 39 & 0x1ff)
#define dir_ptr_component_va(v) ((v) >> 30 & 0x1ff)
#define dir_component_va(v) ((v) >> 21 & 0x1ff)
#define offset_component_va(v) ((v) & 0x1fffff)

/* Clears lower n bits. n is evaluated more than once. */
#define clear_lower_bits(p, n) ((p) >> (n) << (n))


extern uint64_t max_pa_excl;


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


void free_4_level_paging(uint64_t pml4_pa)
{
    /* Assumes that data pages have already been freed. */
    uint64_t i, j, pml4e_pa, pml4e_content, pdpt_pa, pdpte_pa, pdpte_content,
        pd_pa;

    for (i = 0; i < PAGE_TABLE_SIZE; i += BYTES_PER_PAGE_TABLE_ENTRY) {
        /* Level A. */
        pml4e_pa = pml4_pa + i;
        pml4e_content = *(uint64_t *) pa_to_va(pml4e_pa);
        if (pml4e_content & PAGE_PRESENT) {
            pdpt_pa = clear_lower_bits(pml4e_content, 12);
            for (j = 0; j < PAGE_TABLE_SIZE; j += BYTES_PER_PAGE_TABLE_ENTRY) {
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


uint64_t create_kernel_virtual_memory_space(void)
{
    /*
     * Maps all of physical memory into the kernel space.
     * This is similar to the kernel space mapping that was created in the
     * loader.asm file, except that this uses 2 MiB pages instead of 1 GiB
     * pages, and this stops at the limit of physical memory, rather than
     * the arbitrary 512 GiB mapping. This also uses the physical memory
     * allocator to get the RAM to write the paging information to.
     * The kernel already has access to all of the RAM, so this is done
     * to prevent the same RAM from being used elsewhere, and to make the
     * of the paging hierarchy independently free-able.
     */
    uint64_t pml4_pa;

    pml4_pa = allocate_page_pa();
    if (pml4_pa == 0)
        return 0;               /* Error. */

    if (map_range
        (pml4_pa, KERNEL_SPACE_VA, pa_to_va(max_pa_excl), 0,
         (uint32_t) READ_AND_WRITE)) {
        free_4_level_paging(pml4_pa);
        return 0;               /* Error. */
    }

    return pml4_pa;
}


uint64_t create_user_virtual_memory_space(uint64_t exec_start_va,
                                          uint64_t exec_size)
{
    uint64_t pml4_pa, v, p, s, x, y;

    /*
     * Every user space also has a kernel space.
     * This kernel space will be the same. The only reason why it is written to
     * new memory pages is so that the whole paging structure can be freed when
     * the process terminates, without affecting the other processes.
     */
    pml4_pa = create_kernel_virtual_memory_space();
    if (pml4_pa == 0)
        return 0;               /* Error. */


    v = exec_start_va;
    s = exec_size;
    y = USER_EXEC_START_VA;

    while (s) {
        p = allocate_page_pa();
        if (p == 0)
            goto clean_up;

        if (s <= (uint64_t) PAGE_SIZE)
            x = s;
        else
            x = (uint64_t) PAGE_SIZE;

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


    /* User stack. */
    p = allocate_page_pa();
    if (p == 0)
        goto clean_up;

    /* Map the start of the page. */
    if (map_range
        (pml4_pa, USER_STACK_VA - (uint64_t) PAGE_SIZE, USER_STACK_VA, p,
         (uint32_t) READ_AND_WRITE | USER_ACCESS)) {
        free_page_pa(p);
        goto clean_up;
    }


    return pml4_pa;

  clean_up:
    (void) free_user_data_range(pml4_pa, USER_EXEC_START_VA, exec_size);
    free_4_level_paging(pml4_pa);

    return 0;                   /* Error. */
}
