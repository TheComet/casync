#include "casync/casync.h"

void* casync_init_stack(
    void* function,
    void* arg,
    void* return_addr,
    void* stack_buffer,
    int   stack_size)
{
    uint32_t* sp = stack_buffer;
    int       stack_words = stack_size / sizeof(uint32_t);
    sp += stack_words; /* x86 grows downwards */

    /* task_func(void* arg) */
    sp -= 3;                       /* Align stack to 16 bytes before call */
    *--sp = (uint32_t)arg;         /* Task argument 1 */
    *--sp = (uint32_t)return_addr; /* Where to return to when task completes*/

    /* Set up so we "return" to the task function when restoring context */
    *--sp = (uint32_t)function; /* return address to task */

    /* eflags */
    *--sp = 0;

    /* General purpose registers (pusha) */
    sp -= 8;

    return sp;
}
