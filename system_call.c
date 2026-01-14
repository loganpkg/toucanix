#include "interrupt.h"
#include "k_printf.h"
#include "screen.h"
#include "process.h"
#include "defs.h"


extern uint64_t timer_counter;


static int system_write(uint8_t fd, const void *buf, uint64_t s)
{
    switch (fd) {
    case STDOUT_FILENO:
        write_to_screen((char *) buf, (int) s);
        return (int) s;
    }
    return SYS_ERROR;
}


static int system_sleep(uint64_t seconds)
{
    uint64_t tc_orig, events_needed, stop;

    tc_orig = timer_counter;

    if (EVENTS_PER_SECOND && seconds > U64_MAX / EVENTS_PER_SECOND)
        return SYS_ERROR;       /* Overflow. */

    events_needed = seconds * EVENTS_PER_SECOND;

    if (tc_orig > U64_MAX - events_needed) {
        /*
         * Timer needs to wrap around.
         * Complete the remaining time before the wrap around.
         */
        while (timer_counter >= tc_orig)
            sleep(TIMER_WAIT);

        stop = events_needed - (U64_MAX - tc_orig) - 1;
    } else {
        stop = tc_orig + events_needed;
    }

    while (timer_counter < stop)
        sleep(TIMER_WAIT);

    return 0;
}


void system_call(struct interrupt_stack_frame *isf_va)
{
    /*
     * Software system call, kernel side.
     * See user_lib/u_system_call.asm for the user side.
     */

    uint64_t *arg_array = (uint64_t *) isf_va->rsi;

    /* Software system call number. */
    switch (isf_va->rax) {
    case SYS_CALL_WRITE:
        /* Check number of args. */
        if (isf_va->rdi != 3) {
            isf_va->rax = SYS_ERROR;
            return;
        }

        isf_va->rax =
            (uint64_t) system_write(arg_array[0], (void *) arg_array[1],
                                    arg_array[2]);
        break;

    case SYS_CALL_SLEEP:
        /* Check number of args. */
        if (isf_va->rdi != 1) {
            isf_va->rax = SYS_ERROR;
            return;
        }

        isf_va->rax = (uint64_t) system_sleep(arg_array[0]);
        break;

    default:
        isf_va->rax = SYS_ERROR;
        return;
    }
}
