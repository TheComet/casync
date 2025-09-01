#include "casync/casync.h"
#include <stdarg.h>
#include <stddef.h>

struct casync_task
{
    void*        stack;
    struct casync_task* next;
};

struct casync_task* casync_current_task;

void casync_task_switch(void)
{
    casync_current_task = casync_current_task->next;
}

static void finish_task(void)
{
    struct casync_task* t = casync_current_task;
    while (t->next != casync_current_task)
        t = t->next;
    t->next = casync_current_task->next;
    casync_yield();
}

void casync_gather_static(int n, ...)
{
    va_list      ap;
    struct casync_task  tasks[n + 1];
    struct casync_task* this_task = &tasks[n];

    va_start(ap, n);
    while (n--)
    {
        void*  function = va_arg(ap, void*);
        void*  stack = va_arg(ap, void*);
        size_t stack_size = va_arg(ap, size_t);
        void*  arg = va_arg(ap, void*);

        tasks[n].stack =
            casync_init_stack(stack, stack_size, function, arg, finish_task);
        tasks[n + 1].next = &tasks[n];
    }
    va_end(ap);

    /* Close the loop */
    tasks[0].next = this_task;

    while (1)
    {
        /* Run our chain */
        struct casync_task* store_task = casync_current_task;
        casync_current_task = this_task;
        casync_yield();
        casync_current_task = store_task;

        if (this_task == this_task->next)
            break;

        /* Context switch to parent casync_gather(), if one exists */
        if (casync_current_task)
            casync_yield();
    }
}
