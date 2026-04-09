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

#ifndef LL_H
#define LL_H

#ifdef TOUCANIX
#include "defs.h"
#else
#include "test/test_defs.h"
#endif

struct ll_node {
    int data;
    int used;
    int prev;
    int next;
};

struct linked_list {
    int used_count; /* Count of used nodes. */
    int free;       /* Index of next free node. */
    int head;
    int tail;
    struct ll_node list[MAX_NODES];
};

void dump_ll(struct linked_list *y);
void init_ll(struct linked_list *y);
int push_to_head_ll(struct linked_list *y, int data);
int push_to_tail_ll(struct linked_list *y, int data);
int pop_from_head_ll(struct linked_list *y, int *data);
int pop_from_tail_ll(struct linked_list *y, int *data);
int remove_node_ll(struct linked_list *y, int index);

#endif
