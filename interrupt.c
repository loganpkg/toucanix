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
#include "interrupt.h"
#include "screen.h"


#define CODE_SEGMENT_INDEX 1
#define CODE_SELECTOR (CODE_SEGMENT_INDEX << 3)

#define IDT_NUM_ENTRIES 256
#define INTERRUPT_GATE_TYPE 0xe

#define PRESENT_BIT_SET (1 << 7)


/* The Argument vn stands for Vector Number. */
#define set_isr(vn) update_idt_with_isr(idt + vn, (uint64_t) vector_ ## vn, \
    (uint8_t) (PRESENT_BIT_SET | INTERRUPT_GATE_TYPE))


struct idt_entry {
    uint16_t offset_0_to_15;
    uint16_t segment_selector;
    uint8_t reserved_and_ist;   /* Interrupt Stack Table (IST). */
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


struct interrupt_stack_frame {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rbp;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;

    uint64_t vector_number;
    uint64_t error_code;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};




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

void load_idt(struct idt_descriptor *idt_desc_p);
int is_spurious_interrupt(void);
void acknowledge_interrupt(void);




static void update_idt_with_isr(struct idt_entry *idt_e_p,
                                uint64_t address_of_isr,
                                uint8_t present_dpl_gate_type)
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

    idt_desc.idt_size_minus_1 =
        (IDT_NUM_ENTRIES * sizeof(struct idt_entry)) - 1;
    idt_desc.address_of_idt = (uint64_t) idt;
    load_idt(&idt_desc);
}


void interrupt_handler(uint64_t address_of_interrupt_stack_frame)
{
    char *v;
    struct interrupt_stack_frame *isf_p;

    isf_p = (struct interrupt_stack_frame *) address_of_interrupt_stack_frame;

    switch (isf_p->vector_number) {
    case 32:
        /* Timer */

        v = (char *) VIDEO_VIRTUAL_ADDRESS;
        ++*v;
        *((uint8_t *) v + 1) = GREEN;

        acknowledge_interrupt();
        break;
    case 39:
        v = (char *) VIDEO_VIRTUAL_ADDRESS + 2;
        ++*v;
        *((uint8_t *) v + 1) = YELLOW;

        if (is_spurious_interrupt())
            break;
        else
            acknowledge_interrupt();

        break;

    default:
        v = (char *) VIDEO_VIRTUAL_ADDRESS + 4;
        switch (isf_p->vector_number) {
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

        while (1);
    }
}
