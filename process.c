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

#include "address.h"
#include "allocator.h"
#include "asm_lib.h"
#include "defs.h"
#include "interrupt.h"
#include "k_printf.h"
#include "ll.h"
#include "paging.h"
#include "stop.h"

#define KERNEL_PID 0

/* Process states. */
#define UNUSED_PROCESS   0
#define READY_PROCESS    1
#define RUNNING_PROCESS  2
#define SLEEPING_PROCESS 3
#define KILL_PROCESS     4

#define RFLAGS_INTERRUPT_ENABLE (1 << 9)
#define RFLAGS_RESERVED_BIT_1   (1 << 1)

struct process_control_block {
    uint64_t pml4_pa;
    uint64_t kernel_stack_page_va;
    struct interrupt_stack_frame *isf_va;
    /* Used to save the rsp value before process switch. */
    uint64_t rsp_save;

    uint32_t pid;  /* Process Id. */
    uint32_t ppid; /* Parent process Id. */
    uint32_t state;

    int sleep_reason;
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

static struct linked_list ready_list;
static struct linked_list sleep_list;
static struct linked_list kill_list;

static int current_index = -1;

extern struct task_state_segment tss;

static void print_pcb(void)
{
    char *state_str;
    size_t i;

    (void) k_printf("current_index: %ld\n", current_index);

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
            case KILL_PROCESS:
                state_str = "KILL_PROCESS";
                break;
            default:
                state_str = "UNKNOWN";
                break;
            }
            (void) k_printf("state: %s\n", state_str);

            (void) k_printf("pml4_pa: %lx\n", pcb[i].pml4_pa);
            (void) k_printf(
                "kernel_stack_page_va: %lx\n", pcb[i].kernel_stack_page_va);

            (void) k_printf("isf_va: %lx\n", pcb[i].isf_va);
            (void) k_printf("rsp_save: %lu\n", pcb[i].rsp_save);

            (void) k_printf("sleep_reason: %ld\n", pcb[i].sleep_reason);
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
        return -1; /* Failure: No free process slots. */

    if (!(p = allocate_page_pa()))
        return -1;

    pcb[i].kernel_stack_page_va = pa_to_va(p);

    if (!(pcb[i].pml4_pa
            = create_user_virtual_memory_space(pa_to_va(bin_pa), bin_size))) {
        free_page_pa(p);
        return -1;
    }

    pcb[i].isf_va
        = (struct interrupt_stack_frame *) (pcb[i].kernel_stack_page_va
            + PAGE_SIZE - sizeof(struct interrupt_stack_frame));

    /*
     * Prepare the stack for first time entry in the
     * switch_process function.
     */
    pcb[i].rsp_save
        = (uint64_t) pcb[i].isf_va - sizeof(struct switch_stack_frame);
    ((struct switch_stack_frame *) pcb[i].rsp_save)->interrupt_return
        = (uint64_t) interrupt_return;

    pcb[i].isf_va->rip = USER_EXEC_START_VA;
    pcb[i].isf_va->cs = (uint64_t) USER_CODE_SELECTOR;
    pcb[i].isf_va->rflags
        = (uint64_t) (RFLAGS_INTERRUPT_ENABLE | RFLAGS_RESERVED_BIT_1);
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

    pcb[i].state = READY_PROCESS;
    if (push_to_tail_ll(&ready_list, i))
        return -1;

    (void) print_pcb();

    return 0;
}

int start_init_process(void)
{
    /*
     * Start the first process, called init.
     * Prepares other user processes too.
     */

    /* Clear tss struct. */
    memset(&tss, 0, sizeof(struct task_state_segment));

    /* Clear process array. */
    memset(pcb, 0, sizeof(struct process_control_block) * MAX_PROCESSES);

    init_ll(&ready_list);
    init_ll(&sleep_list);
    init_ll(&kill_list);

    /* This is the init process. */
    if (prepare_process(USER_A_PA, USER_A_SIZE))
        return -1;

    /* Other user processes. */
    if (prepare_process(USER_B_PA, USER_B_SIZE))
        return -1;

    if (prepare_process(USER_C_PA, USER_C_SIZE))
        return -1;

    /* Must be at least one process to start. */
    if (!ready_list.used_count)
        return -1;

    /* The init process. */
    if (pop_from_head_ll(&ready_list, &current_index))
        return -1;

    if (pcb[current_index].state != READY_PROCESS)
        return -1;

    pcb[current_index].state = RUNNING_PROCESS;

    tss.rsp0 = pcb[current_index].kernel_stack_page_va + PAGE_SIZE;
    switch_pml4_pa(pcb[current_index].pml4_pa);

    (void) k_printf("About to enter process...\n");

    enter_process(pcb[current_index].isf_va);
    return 0;
}

static void schedule(void)
{
    int old_current_index;

    /* No ready processes. */
    stop(!ready_list.used_count);

    old_current_index = current_index;

    stop(pop_from_head_ll(&ready_list, &current_index));
    stop(pcb[current_index].state != READY_PROCESS);

    pcb[current_index].state = RUNNING_PROCESS;

    tss.rsp0 = pcb[current_index].kernel_stack_page_va + PAGE_SIZE;
    switch_pml4_pa(pcb[current_index].pml4_pa);

    switch_process(
        &pcb[old_current_index].rsp_save, pcb[current_index].rsp_save);
}

void give_up_execution(void)
{
    stop(push_to_tail_ll(&ready_list, current_index));
    pcb[current_index].state = READY_PROCESS;

    schedule();
}

void sleep(int sleep_reason)
{
    stop(push_to_head_ll(&sleep_list, current_index));
    pcb[current_index].state = SLEEPING_PROCESS;
    pcb[current_index].sleep_reason = sleep_reason;

    schedule();
}

void wake_up(int sleep_reason)
{
    /*
     * Wake up all processes that are asleep for a given reason.
     * Push them into the ready list.
     */

    int i, next;
    struct linked_list *w = &sleep_list;

    /* Walk the linked list. */
    i = w->head;
    while (i != -1) {
        next = w->list[i].next; /* Save as the node might get deleted. */

        if (pcb[w->list[i].data].sleep_reason == sleep_reason) {
            stop(pcb[w->list[i].data].state != SLEEPING_PROCESS);

            stop(push_to_head_ll(&ready_list, w->list[i].data));
            pcb[w->list[i].data].state = READY_PROCESS;

            stop(remove_node_ll(&sleep_list, i));
        }
        i = next;
    }
}

void exit(void)
{
    stop(push_to_tail_ll(&kill_list, current_index));
    pcb[current_index].state = KILL_PROCESS;

    wake_up(INIT_PROCESS_SLEEP);
    schedule();
}

void clean_up(void)
{
    /* Called by init process. Cleans up all killed processes. */
    int i, index;

    while (1) {
        i = kill_list.head;
        while (i != -1) {
            stop(pop_from_head_ll(&kill_list, &index));
            stop(pcb[index].state != KILL_PROCESS);

            /* Clean up. */
            free_page_pa(va_to_pa(pcb[index].kernel_stack_page_va));
            free_4_level_paging(pcb[index].pml4_pa);

            memset(pcb + index, 0, sizeof(struct process_control_block));

            i = kill_list.list[i].next;
        }

        sleep(INIT_PROCESS_SLEEP);
    }
}
