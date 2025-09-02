#include "casync/casync.h"

void* casync_init_stack(
    void* function,
    void* arg,
    void* return_addr,
    void* stack_buffer,
    int   stack_size)
{
    uint64_t* sp = stack_buffer;
    int       stack_words = stack_size / sizeof(uint64_t);
    sp += stack_words; /* x86 grows downwards */

    sp -= 4;                       /* 32 bytes of shadow space (win64) */
    *--sp = (uint64_t)return_addr; /* Where to return to when task completes*/

    /* Set up so we "return" to the coroutine function when restoring context */
    *--sp = (uint64_t)function;

    /* General purpose registers */
    --sp;                  /* rax */
    *--sp = (uint64_t)arg; /* rcx */
    --sp;                  /* rdx */
    --sp;                  /* rbx */
    --sp;                  /* rbp */
    --sp;                  /* rsi */
    --sp;                  /* rdi */
    sp -= 8;               /* r8 - r15 */

    /* flags register */
    *--sp = 0;

    return sp;
}
