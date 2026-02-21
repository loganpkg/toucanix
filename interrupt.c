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

#include "interrupt.h"
#include "address.h"
#include "asm_lib.h"
#include "defs.h"
#include "k_printf.h"
#include "process.h"
#include "screen.h"
#include "system_call.h"

#define IDT_NUM_ENTRIES     256
#define INTERRUPT_GATE_TYPE 0xe

#define CPL_MASK 3

/* The Argument vn stands for Vector Number. */
#define set_isr(vn)                                                           \
    update_idt_with_isr(idt + vn, (uint64_t) vector_##vn,                     \
        (uint8_t) (PRESENT_BIT_SET | INTERRUPT_GATE_TYPE))

struct idt_entry {
    uint16_t offset_0_to_15;
    uint16_t segment_selector;
    uint8_t reserved_and_ist; /* Interrupt Stack Table (IST). */
    uint8_t present_dpl_gate_type;
    uint16_t offset_16_to_31;
    uint32_t offset_32_to_63;
    uint32_t reserved;
} __attribute__((packed));

static struct idt_entry idt[IDT_NUM_ENTRIES];

struct idt_descriptor {
    uint16_t idt_size_minus_1;
    uint64_t address_of_idt;
} __attribute__((packed));

static struct idt_descriptor idt_desc;
uint64_t timer_counter = 0;

/* Functions from the interrupt.asm file. */
extern void vector_0(void);
extern void vector_1(void);
extern void vector_2(void);
extern void vector_3(void);
extern void vector_4(void);
extern void vector_5(void);
extern void vector_6(void);
extern void vector_7(void);
extern void vector_8(void);
/* Vector 9 is reserved. */
extern void vector_10(void);
extern void vector_11(void);
extern void vector_12(void);
extern void vector_13(void);
extern void vector_14(void);
/* Vector 15 is reserved. */
extern void vector_16(void);
extern void vector_17(void);
extern void vector_18(void);
extern void vector_19(void);

extern void vector_32(void);
extern void vector_39(void);

extern void system_software_interrupt(void);

void load_idt(struct idt_descriptor *idt_desc_p);
int is_spurious_interrupt(void);
void acknowledge_interrupt(void);
uint64_t get_cr2(void);

static void update_idt_with_isr(struct idt_entry *idt_e_p,
    uint64_t address_of_isr, uint8_t present_dpl_gate_type)
{
    /*
     * IDT = Interrupt Descriptor Table.
     * ISR = Interrupt Service Routine. Not to be confused with the
     *     In-Service Register (ISR) of the PIC.
     */
    idt_e_p->offset_0_to_15 = (uint16_t) address_of_isr;
    idt_e_p->offset_16_to_31 = (uint16_t) (address_of_isr >> 16);
    idt_e_p->offset_32_to_63 = (uint32_t) (address_of_isr >> 32);

    idt_e_p->segment_selector = (uint16_t) CODE_SELECTOR;
    idt_e_p->reserved_and_ist = 0;
    idt_e_p->present_dpl_gate_type = present_dpl_gate_type;
    idt_e_p->reserved = 0;
}

void init_idt(void)
{
    set_isr(0);
    set_isr(1);
    set_isr(2);
    set_isr(3);
    set_isr(4);
    set_isr(5);
    set_isr(6);
    set_isr(7);
    set_isr(8);
    /* Vector 9 is reserved. */
    set_isr(10);
    set_isr(11);
    set_isr(12);
    set_isr(13);
    set_isr(14);
    /* Vector 15 is reserved. */
    set_isr(16);
    set_isr(17);
    set_isr(18);
    set_isr(19);

    set_isr(32);
    set_isr(39);

    update_idt_with_isr(idt + SOFTWARE_INT,
        (uint64_t) system_software_interrupt,
        (uint8_t) (PRESENT_BIT_SET | DESCRIPTOR_PRIVILEGE_LEVEL_USER
            | INTERRUPT_GATE_TYPE));

    idt_desc.idt_size_minus_1
        = (IDT_NUM_ENTRIES * sizeof(struct idt_entry)) - 1;
    idt_desc.address_of_idt = (uint64_t) idt;
    load_idt(&idt_desc);
}

void interrupt_handler(uint64_t address_of_interrupt_stack_frame)
{
    char *v;
    struct interrupt_stack_frame *isf_va;

    isf_va = (struct interrupt_stack_frame *) address_of_interrupt_stack_frame;

    switch (isf_va->vector_number) {
    case 32:
        /* Timer */

        v = (char *) VIDEO_VA;
        ++*v;
        *((uint8_t *) v + 1) = BLUE;

        acknowledge_interrupt();
        /*
         * Interrupts are disabled in kernel mode, so if all processes are
         * stuck in kernel mode (such as sleep), this will never increment.
         */
        ++timer_counter;
        wake(TIMER_WAIT);
        schedule();
        break;
    case 39:
        v = (char *) VIDEO_VA + 2;
        ++*v;
        *((uint8_t *) v + 1) = YELLOW;

        if (is_spurious_interrupt())
            break;
        else
            acknowledge_interrupt();

        break;

    case SOFTWARE_INT:
        system_call(isf_va);
        break;

    default:
        v = (char *) VIDEO_VA + 4;
        switch (isf_va->vector_number) {
        case 0:
            *v = '0';
            break;
        case 1:
            *v = '1';
            break;
        case 2:
            *v = '2';
            break;
        case 3:
            *v = '3';
            break;
        case 4:
            *v = '4';
            break;
        case 5:
            *v = '5';
            break;
        case 6:
            *v = '6';
            break;
        case 7:
            *v = '7';
            break;
        case 8:
            *v = '8';
            break;
        case 10:
            *v = 'A';
            break;
        case 11:
            *v = 'B';
            break;
        case 12:
            *v = 'C';
            break;
        case 13:
            *v = 'D';
            break;
        case 14:
            *v = 'E';
            break;
        case 16:
            *v = 'G';
            break;
        case 17:
            *v = 'H';
            break;
        case 18:
            *v = 'I';
            break;
        case 19:
            *v = 'J';
            break;
        }

        *((uint8_t *) v + 1) = RED;

        (void) k_printf("Interrupt Handler:\n");
        (void) k_printf(
            "    Vector Number: %lu\n", (unsigned long) isf_va->vector_number);
        (void) k_printf(
            "    Error Code: %lu\n", (unsigned long) isf_va->error_code);
        (void) k_printf(
            "    Ring: %lu\n", (unsigned long) (isf_va->cs & CPL_MASK));
        (void) k_printf("    rip: %lx\n", (unsigned long) isf_va->rip);
        (void) k_printf("    cr2: %lx\n", (unsigned long) get_cr2());

        while (1);
    }
}
