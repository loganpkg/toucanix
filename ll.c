/*
 * Copyright (c) 2026 Logan Ryan McLintock. All rights reserved.
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

/* [Doubly] Linked List using static memory. */

#include "ll.h"

#ifdef TOUCANIX
#include "k_printf.h"
#include "stdint.h"
#else
#include <stdint.h>
#include <stdio.h>
#endif

void dump_ll(struct linked_list *y)
{
    int i;
    k_printf("============\n");
    k_printf("used_count: %ld\n", (int64_t) y->used_count);
    k_printf("free: %ld\n", (int64_t) y->free);
    k_printf("head: %ld\n", (int64_t) y->head);
    k_printf("tail: %ld\n", (int64_t) y->tail);

    k_printf("Index:");
    for (i = 0; i < MAX_NODES; ++i) k_printf("\t%ld", (int64_t) i);

    k_printf("\ndata:");
    for (i = 0; i < MAX_NODES; ++i)
        k_printf("\t%ld", (int64_t) y->list[i].data);

    k_printf("\nused:");
    for (i = 0; i < MAX_NODES; ++i)
        k_printf("\t%ld", (int64_t) y->list[i].used);

    k_printf("\nprev:");
    for (i = 0; i < MAX_NODES; ++i)
        k_printf("\t%ld", (int64_t) y->list[i].prev);

    k_printf("\nnext:");
    for (i = 0; i < MAX_NODES; ++i)
        k_printf("\t%ld", (int64_t) y->list[i].next);

    i = y->head;
    k_printf("\nList forward:\n");
    if (i == -1) {
        k_printf("Empty list");
    } else {
        k_printf("%ld", (int64_t) y->list[i].data);
        i = y->list[i].next;
    }
    while (i != -1) {
        k_printf(" -> %ld", (int64_t) y->list[i].data);
        i = y->list[i].next;
    }

    i = y->tail;
    k_printf("\nList backwards:\n");
    if (i == -1) {
        k_printf("Empty list");
    } else {
        k_printf("%ld", (int64_t) y->list[i].data);
        i = y->list[i].prev;
    }
    while (i != -1) {
        k_printf(" -> %ld", (int64_t) y->list[i].data);
        i = y->list[i].prev;
    }

    k_printf("\n============\n");
}

void init_ll(struct linked_list *y)
{
    int i;
    y->used_count = 0;
    y->free = 0;
    y->head = -1;
    y->tail = -1;
    for (i = 0; i < MAX_NODES; ++i) {
        y->list[i].data = 0;
        y->list[i].used = 0;
        y->list[i].prev = -1;
        y->list[i].next = -1;
    }
}

int push_to_head_ll(struct linked_list *y, int data)
{
    int i;
    if (y->used_count == MAX_NODES)
        return 1; /* Full. */

    y->list[y->free].data = data;
    y->list[y->free].used = 1;

    if (y->head != -1) {
        y->list[y->free].next = y->head;
        y->list[y->head].prev = y->free;
    }

    y->head = y->free;

    if (y->tail == -1)
        y->tail = y->free;

    ++y->used_count;

    if (y->used_count == MAX_NODES) {
        y->free = -1; /* There are no free nodes. */
        return 0;
    }

    /* Find next free node. */
    for (i = y->free + 1; i < MAX_NODES; ++i)
        if (!y->list[i].used) {
            y->free = i;
            return 0;
        }
    for (i = 0; i < y->free; ++i)
        if (!y->list[i].used) {
            y->free = i;
            return 0;
        }

    return 0;
}

int push_to_tail_ll(struct linked_list *y, int data)
{
    int i;
    if (y->used_count == MAX_NODES)
        return 1; /* Full. */

    y->list[y->free].data = data;
    y->list[y->free].used = 1;

    if (y->tail != -1) {
        y->list[y->free].prev = y->tail;
        y->list[y->tail].next = y->free;
    }

    y->tail = y->free;

    if (y->head == -1)
        y->head = y->free;

    ++y->used_count;

    if (y->used_count == MAX_NODES) {
        y->free = -1; /* There are no free nodes. */
        return 0;
    }

    /* Find next free node. */
    for (i = y->free + 1; i < MAX_NODES; ++i)
        if (!y->list[i].used) {
            y->free = i;
            return 0;
        }
    for (i = 0; i < y->free; ++i)
        if (!y->list[i].used) {
            y->free = i;
            return 0;
        }

    return 0;
}

int pop_from_head_ll(struct linked_list *y, int *data)
{
    int t;
    if (!y->used_count)
        return 1; /* Empty list. */

    *data = y->list[y->head].data;
    y->list[y->head].data = 0;
    y->list[y->head].used = 0;

    t = y->list[y->head].next;
    y->list[y->head].next = -1;

    if (t != -1)
        y->list[t].prev = -1;

    y->free = y->head;
    --y->used_count;

    y->head = t;

    if (y->head == -1) {
        /* Empty list. */
        y->tail = -1;
        y->free = 0;
        if (y->used_count)
            return 1;
    }

    return 0;
}

int pop_from_tail_ll(struct linked_list *y, int *data)
{
    int t;
    if (!y->used_count)
        return 1; /* Empty list. */

    *data = y->list[y->tail].data;
    y->list[y->tail].data = 0;
    y->list[y->tail].used = 0;

    t = y->list[y->tail].prev;
    y->list[y->tail].prev = -1;

    if (t != -1)
        y->list[t].next = -1;

    y->free = y->tail;
    --y->used_count;

    y->tail = t;

    if (y->tail == -1) {
        /* Empty list. */
        y->head = -1;
        y->free = 0;
        if (y->used_count)
            return 1;
    }

    return 0;
}

int remove_node_ll(struct linked_list *y, int index)
{
    if (index < 0 || index >= MAX_NODES)
        return 1;

    if (index == y->head)
        y->head = y->list[index].next;

    if (index == y->tail)
        y->tail = y->list[index].prev;

    y->list[index].data = 0;
    y->list[index].used = 0;

    /* Link around the node: */
    if (y->list[index].prev != -1)
        y->list[y->list[index].prev].next = y->list[index].next;

    if (y->list[index].next != -1)
        y->list[y->list[index].next].prev = y->list[index].prev;

    y->list[index].prev = -1;
    y->list[index].next = -1;

    --y->used_count;

    if (!y->used_count)
        y->free = 0;
    else
        y->free = index;

    return 0;
}
