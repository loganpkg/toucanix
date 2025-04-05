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

#include "address.h"
#include "asm_lib.h"
#include "interrupt.h"
#include "memory.h"


#define MAX_PROCESSES 1024

#define KERNEL_PID 0
#define INIT_PID 1

/* Process states. */
#define UNUSED_PROCESS 0
#define INIT_PROCESS 1


#define USER_RING 3

/* GDT. */
#define USER_CODE_SEGMENT_INDEX 2
#define USER_DATA_SEGMENT_INDEX 3
#define USER_CODE_SELECTOR (USER_CODE_SEGMENT_INDEX << 3 | USER_RING)
#define USER_DATA_SELECTOR (USER_DATA_SEGMENT_INDEX << 3 | USER_RING)

#define RFLAGS_INTERRUPT_ENABLE (1 << 9)
#define RFLAGS_RESERVED_BIT_1 (1 << 1)

#define TSS_SIZE 104


struct process_control_block {
    uint32_t pid;               /* Process Id. */
    uint32_t ppid;              /* Parent process Id. */
    uint32_t state;
    uint64_t pml4_pa;
    uint64_t kernel_stack_page_va;
    struct interrupt_stack_frame *isf_va;
};


struct task_state_segment {
    uint32_t reserved;
    uint64_t rsp0;
    unsigned char unused[TSS_SIZE - sizeof(uint32_t) - sizeof(uint64_t)];
}
__attribute__((packed));


static struct process_control_block pcb[MAX_PROCESSES];

extern struct task_state_segment tss;


static int main(void);


static int get_unused_process_index(void)
{
    int i;

    for (i = 0; i < MAX_PROCESSES; ++i) {
        if (pcb[i].state == UNUSED_PROCESS)
            return i;
    }

    return -1;                  /* Error. */
}


int start_init_process(void)
{
    /* This is first process from which all other processes will be forked. */
    int i;
    struct process_control_block *pr;
    uint64_t p;

    memset(pcb, 0, sizeof(struct process_control_block) * MAX_PROCESSES);

    if ((i = get_unused_process_index()) == -1)
        return -1;

    pr = &pcb[i];

    pr->pid = INIT_PID;
    pr->ppid = KERNEL_PID;

    pr->state = INIT_PROCESS;

    if (!(p = allocate_page_pa()))
        return -1;

    pr->kernel_stack_page_va = pa_to_va(p);

    if (!
        (pr->pml4_pa =
         create_user_virtual_memory_space((uint64_t) main, 10485773))) {
        free_page_pa(p);
        return -1;
    }

    pr->isf_va =
        (struct interrupt_stack_frame *) (pr->kernel_stack_page_va +
                                          PAGE_SIZE -
                                          sizeof(struct
                                                 interrupt_stack_frame));
    pr->isf_va->rip = USER_EXEC_START_VA;
    pr->isf_va->cs = (uint64_t) USER_CODE_SELECTOR;
    pr->isf_va->rflags =
        (uint64_t) (RFLAGS_INTERRUPT_ENABLE | RFLAGS_RESERVED_BIT_1);
    pr->isf_va->rsp = (uint64_t) (USER_STACK_PAGE_VA + PAGE_SIZE);
    pr->isf_va->ss = (uint64_t) USER_DATA_SELECTOR;

    tss.rsp0 = pr->kernel_stack_page_va + PAGE_SIZE;

    switch_pml4_pa(pr->pml4_pa);

    enter_process(pr->isf_va);

    return 0;
}

static int main(void)
{
    char x = 'X';
    ++x;
    *((char *) VIDEO_VA + 10) = x;
    return 0;
}
