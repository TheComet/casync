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

    /* Where to return to when task completes*/
    *--sp = (uint64_t)return_addr;

    /* Set up so we "return" to the task function when restoring context */
    *--sp = (uint64_t)function; /* return address to task */

    /* General purpose registers */
    *--sp = 0;             /* rax */
    *--sp = 0;             /* rcx */
    *--sp = 0;             /* rdx */
    *--sp = 0;             /* rbx */
    *--sp = 0;             /* rbp */
    *--sp = 0;             /* rsi */
    *--sp = (uint64_t)arg; /* rdi */
    sp -= 8;               /* r8 - r15 */

    /* flags */
    *--sp = 0x00000000;

    return sp;
}
