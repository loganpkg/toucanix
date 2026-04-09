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

/* Test [Doubly] Linked List. */

#include "../ll.h"
#include <stdio.h>

int main(void)
{
    int x, i;
    struct linked_list y;

    init_ll(&y);

    dump_ll(&y);

    x = 10;
    printf("Push to head: %d\n", x);
    push_to_head_ll(&y, x);
    dump_ll(&y);

    x = 20;
    printf("Push to head: %d\n", x);
    push_to_head_ll(&y, x);
    dump_ll(&y);

    x = 30;
    printf("Push to head: %d\n", x);
    push_to_head_ll(&y, x);
    dump_ll(&y);

    i = 1;
    printf("Remove node: %d\n", i);
    remove_node_ll(&y, i);
    dump_ll(&y);

    x = 40;
    printf("Push to tail: %d\n", x);
    push_to_tail_ll(&y, x);
    dump_ll(&y);

    x = 50;
    printf("Push to tail: %d\n", x);
    push_to_tail_ll(&y, x);
    dump_ll(&y);

    i = y.tail;
    printf("Remove node: %d\n", i);
    remove_node_ll(&y, i);
    dump_ll(&y);

    x = 60;
    printf("Push to tail: %d\n", x);
    push_to_tail_ll(&y, x);
    dump_ll(&y);

    printf("Pop from head\n");
    pop_from_head_ll(&y, &x);
    printf("x: %d\n", x);
    dump_ll(&y);

    printf("Pop from tail\n");
    pop_from_tail_ll(&y, &x);
    printf("x: %d\n", x);
    dump_ll(&y);

    printf("Pop from head\n");
    pop_from_head_ll(&y, &x);
    printf("x: %d\n", x);
    dump_ll(&y);

    i = y.head;
    printf("Remove node: %d\n", i);
    remove_node_ll(&y, i);
    dump_ll(&y);

    x = 80;
    printf("Push to tail: %d\n", x);
    push_to_tail_ll(&y, x);
    dump_ll(&y);

    printf("Pop from tail\n");
    pop_from_tail_ll(&y, &x);
    printf("x: %d\n", x);
    dump_ll(&y);

    return 0;
}
