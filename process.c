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


#include "defs.h"
#include "address.h"
#include "asm_lib.h"
#include "interrupt.h"
#include "allocator.h"
#include "paging.h"
#include "k_printf.h"


#define MAX_PROCESSES 1024

#define KERNEL_PID 0

/* Process states. */
#define UNUSED_PROCESS 0
#define READY_PROCESS 1
#define RUNNING_PROCESS 2
#define SLEEPING_PROCESS 3


#define RFLAGS_INTERRUPT_ENABLE (1 << 9)
#define RFLAGS_RESERVED_BIT_1 (1 << 1)


struct process_control_block {
    uint32_t pid;               /* Process Id. */
    uint32_t ppid;              /* Parent process Id. */
    uint32_t state;
    uint64_t pml4_pa;
    uint64_t kernel_stack_page_va;
    struct interrupt_stack_frame *isf_va;
    /* Used to save the rsp value before process switch. */
    uint64_t rsp_save;
    int wait_reason;
    int wait_next;              /* Index of the next sleeping process. */
    int ready_next;             /* Index of the next ready process. */
};


struct task_state_segment {
    uint32_t reserved;
    uint64_t rsp0;
    unsigned char unused[TSS_SIZE - sizeof(uint32_t) - sizeof(uint64_t)];
} __attribute__((packed));


struct switch_stack_frame {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t rbp;
    uint64_t rbx;
    uint64_t interrupt_return;
};


static struct process_control_block pcb[MAX_PROCESSES];

static int current_index = -1;
static int ready_head_index = -1;
static int ready_tail_index = -1;
static int wait_head_index = -1;


extern struct task_state_segment tss;


static void print_pcb(void)
{
    char *state_str;
    size_t i;

    if (current_index == -1)
        (void) k_printf("current_index: %s\n", "-1");
    else
        (void) k_printf("current_index: %lu\n", current_index);

    if (ready_head_index == -1)
        (void) k_printf("ready_head_index: %s\n", "-1");
    else
        (void) k_printf("ready_head_index: %lu\n", ready_head_index);

    if (ready_tail_index == -1)
        (void) k_printf("ready_tail_index: %s\n", "-1");
    else
        (void) k_printf("ready_tail_index: %lu\n", ready_tail_index);

    if (wait_head_index == -1)
        (void) k_printf("wait_head_index: %s\n", "-1");
    else
        (void) k_printf("wait_head_index: %lu\n", wait_head_index);


    for (i = 0; i < MAX_PROCESSES; ++i) {
        if (pcb[i].state != UNUSED_PROCESS) {
            (void) k_printf("%s\n", "------------");
            (void) k_printf("pid: %lu\n", pcb[i].pid);
            (void) k_printf("ppid: %lu\n", pcb[i].ppid);

            switch (pcb[i].state) {
            case READY_PROCESS:
                state_str = "READY_PROCESS";
                break;
            case RUNNING_PROCESS:
                state_str = "RUNNING_PROCESS";
                break;
            case SLEEPING_PROCESS:
                state_str = "SLEEPING_PROCESS";
                break;
            }
            (void) k_printf("state: %s\n", state_str);

            (void) k_printf("pml4_pa: %lx\n", pcb[i].pml4_pa);
            (void) k_printf("kernel_stack_page_va: %lx\n",
                            pcb[i].kernel_stack_page_va);

            (void) k_printf("isf_va: %lx\n", pcb[i].isf_va);
            (void) k_printf("rsp_save: %lu\n", pcb[i].rsp_save);

            if (pcb[i].wait_reason == -1)
                (void) k_printf("wait_reason: %s\n", "-1");
            else
                (void) k_printf("wait_reason: %lu\n", pcb[i].wait_reason);

            if (pcb[i].wait_next == -1)
                (void) k_printf("wait_next: %s\n", "-1");
            else
                (void) k_printf("wait_next: %lu\n", pcb[i].wait_next);

            if (pcb[i].ready_next == -1)
                (void) k_printf("ready_next: %s\n", "-1");
            else
                (void) k_printf("ready_next: %lu\n", pcb[i].ready_next);
        }
    }
}


static int prepare_process(uint64_t bin_pa, uint64_t bin_size)
{
    int i;
    uint64_t p;

    for (i = 0; i < MAX_PROCESSES; ++i)
        if (pcb[i].state == UNUSED_PROCESS)
            break;

    if (i == MAX_PROCESSES)
        return -1;              /* Failure: No free process slots. */


    if (!(p = allocate_page_pa()))
        return -1;

    pcb[i].kernel_stack_page_va = pa_to_va(p);

    if (!
        (pcb[i].pml4_pa =
         create_user_virtual_memory_space(pa_to_va(bin_pa), bin_size))) {
        free_page_pa(p);
        return -1;
    }

    pcb[i].isf_va =
        (struct interrupt_stack_frame *) (pcb[i].kernel_stack_page_va +
                                          PAGE_SIZE -
                                          sizeof(struct
                                                 interrupt_stack_frame));


    /*
     * Prepare the stack for first time entry in the
     * switch_process function.
     */
    pcb[i].rsp_save =
        (uint64_t) pcb[i].isf_va - sizeof(struct switch_stack_frame);
    ((struct switch_stack_frame *) pcb[i].rsp_save)->interrupt_return =
        (uint64_t) interrupt_return;


    pcb[i].isf_va->rip = USER_EXEC_START_VA;
    pcb[i].isf_va->cs = (uint64_t) USER_CODE_SELECTOR;
    pcb[i].isf_va->rflags =
        (uint64_t) (RFLAGS_INTERRUPT_ENABLE | RFLAGS_RESERVED_BIT_1);
    pcb[i].isf_va->rsp = (uint64_t) USER_STACK_VA;
    pcb[i].isf_va->ss = (uint64_t) USER_DATA_SELECTOR;


    /* Set pid. pids loop within the same array index. */
    if (!pcb[i].pid || pcb[i].pid > U32_MAX - MAX_PROCESSES)
        pcb[i].pid = (uint32_t) i;

    pcb[i].pid += MAX_PROCESSES;

    /* Set ppid. */
    if (current_index == -1)
        pcb[i].ppid = KERNEL_PID;
    else
        pcb[i].ppid = pcb[current_index].pid;

    /* Close off link. */
    pcb[i].ready_next = -1;
    pcb[i].state = READY_PROCESS;

    if (ready_head_index == -1) {
        /* Empty list, so make it the new head (and tail). */
        ready_head_index = i;
        ready_tail_index = i;
    } else {
        /* Add on to ready tail. */
        pcb[ready_tail_index].ready_next = i;
        ready_tail_index = i;
    }

    (void) print_pcb();

    return 0;
}


int start_init_process(void)
{
    /* Start the first process. */

    /* Clear tss struct. */
    memset(&tss, 0, sizeof(struct task_state_segment));

    /* Clear process array. */
    memset(pcb, 0, sizeof(struct process_control_block) * MAX_PROCESSES);


    if (prepare_process(USER_A_PA, USER_A_SIZE))
        return -1;

    if (prepare_process(USER_B_PA, USER_B_SIZE))
        return -1;


    /* Must be at least one process to start. */
    if (ready_head_index == -1)
        return -1;

    current_index = ready_head_index;
    ready_head_index = pcb[ready_head_index].ready_next;

    if (ready_head_index == -1)
        ready_tail_index = -1;  /* Update tail if list is now empty. */

    pcb[current_index].state = RUNNING_PROCESS;

    tss.rsp0 = pcb[current_index].kernel_stack_page_va + PAGE_SIZE;
    switch_pml4_pa(pcb[current_index].pml4_pa);

    (void) k_printf("About to enter process...\n");

    enter_process(pcb[current_index].isf_va);
    return 0;
}


void schedule(void)
{
    /* Do nothing if only the current process. */
    if (ready_head_index == -1)
        return;

    /* Close off link. */
    pcb[current_index].ready_next = -1;

    /* Place current process on the tail. */
    pcb[current_index].state = READY_PROCESS;

    pcb[ready_tail_index].ready_next = current_index;
    ready_tail_index = current_index;

    /* Load the head to the current. */
    current_index = ready_head_index;

    ready_head_index = pcb[current_index].ready_next;
    pcb[current_index].state = RUNNING_PROCESS;

    tss.rsp0 = pcb[current_index].kernel_stack_page_va + PAGE_SIZE;
    switch_pml4_pa(pcb[current_index].pml4_pa);

    switch_process(&pcb[ready_tail_index].rsp_save,
                   pcb[current_index].rsp_save);
}


void sleep(int wait_reason)
{
    pcb[current_index].state = SLEEPING_PROCESS;
    pcb[current_index].wait_reason = wait_reason;
    pcb[current_index].ready_next = -1;
    pcb[current_index].wait_next = wait_head_index;
    wait_head_index = current_index;

    schedule();
}


void wake(int wait_reason)
{
    /*
     * Wake up all processes that are asleep for a given reason.
     * Insert then at the head of the ready list.
     */

    int i_prev, i, j;

    i_prev = -1;
    i = wait_head_index;
    while (i != -1) {
        if (pcb[i].wait_reason == wait_reason) {
            /* Remove process from wait list. */
            if (i == wait_head_index)
                wait_head_index = pcb[i].wait_next;

            /* Bypass the removed process. Relink around it. */
            if (i_prev != -1)
                pcb[i_prev].wait_next = pcb[i].wait_next;


            /* Add process to head of the ready list. */
            pcb[i].wait_reason = 0;
            pcb[i].state = READY_PROCESS;
            pcb[i].ready_next = ready_head_index;

            ready_head_index = i;
            if (ready_tail_index == -1)
                ready_tail_index = i;

            /* Record previous. */
            i_prev = i;

            j = i;
            i = pcb[i].wait_next;
            pcb[j].wait_next = -1;      /* Close off old link. */
        } else {
            i = pcb[i].wait_next;
        }
    }
}
