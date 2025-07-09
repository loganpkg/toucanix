#include "interrupt.h"
#include "k_printf.h"
#include "screen.h"
#include "defs.h"

static int system_write(uint8_t fd, const void *buf, uint64_t s)
{
    switch (fd) {
    case STDOUT_FILENO:
        write_to_screen((char *) buf, (int) s);
        return (int) s;
    }
    return SYS_ERROR;
}

void system_call(struct interrupt_stack_frame *isf_va)
{
    /*
     * Software system call, kernel side.
     * See user_lib/u_system_call.asm for the user side.
     */

    uint64_t *arg_array = (uint64_t *) isf_va->rsi;

    (void) k_printf("In system call kernel side...\n");

    /* Software system call number. */
    switch (isf_va->rax) {
    case SYS_CALL_WRITE:
        /* Number of args. */
        if (isf_va->rdi != 3) {
            isf_va->rax = SYS_ERROR;
            return;
        }

        isf_va->rax =
            (uint64_t) system_write(arg_array[0], (void *) arg_array[1],
                                    arg_array[2]);
        break;
    default:
        isf_va->rax = SYS_ERROR;
        return;
    }
}
