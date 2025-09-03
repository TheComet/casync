#include "casync/casync.h"

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>

#if defined(_MSC_VER)
#    define THREADLOCAL __declspec(thread)
#else
#    define THREADLOCAL __thread
#endif

struct casync_task
{
    void*               stack;
    struct casync_task* next;
    size_t              stack_size;
};

struct casync_loop
{
    struct casync_task* active;
    struct casync_task* finished;
    struct casync_task  control_task;
    int                 return_code;
};

THREADLOCAL struct casync_loop* casync_current_loop;

void casync_end_redirect(void);
void casync_restore(void);

/* -------------------------------------------------------------------------- */
void casync_end(int return_code)
{
    struct casync_task* t = casync_current_loop->active;
    if (return_code != 0)
        casync_current_loop->return_code = return_code;

    /* Take current task out of the loop */
    struct casync_task* prev = t;
    while (prev->next != t)
        prev = prev->next;
    prev->next = t->next;

    /* Insert active task into finished list */
    t->next = casync_current_loop->finished;
    casync_current_loop->finished = t;

    /* Switch to next active task */
    casync_current_loop->active = prev->next;
    casync_restore();
}

/* -------------------------------------------------------------------------- */
static void loop_schedule(struct casync_loop* loop, struct casync_task* task)
{
    struct casync_task* prev = loop->active;
    while (prev->next != loop->active)
        prev = prev->next;
    prev->next = task;
    task->next = loop->active;
}

/* -------------------------------------------------------------------------- */
static void
loop_start_static(struct casync_loop* loop, int (*function)(void*), void* arg)
{
    struct casync_task* task = loop->finished;
    assert(task != NULL);
    loop->finished = loop->finished->next;
    task->stack = casync_init_stack(
        function, arg, casync_end_redirect, task, task->stack_size);

    loop_schedule(loop, task);
}

/* -------------------------------------------------------------------------- */
static void
loop_start(struct casync_loop* loop, int (*function)(void*), void* arg)
{
    struct casync_task* task = loop->finished;
    if (task != NULL)
        loop->finished = loop->finished->next;
    else
    {
        task = malloc(1024 * 1024);
        task->stack_size = 1024 * 1024;
    }
    task->stack = casync_init_stack(
        function, arg, casync_end_redirect, task, task->stack_size);

    loop_schedule(loop, task);
}

/* -------------------------------------------------------------------------- */
void casync_start_static(int (*function)(void*), void* arg)
{
    loop_start_static(casync_current_loop, function, arg);
}

/* -------------------------------------------------------------------------- */
void casync_start(int (*function)(void*), void* arg)
{
    loop_start(casync_current_loop, function, arg);
}

/* -------------------------------------------------------------------------- */
static int casync_run_loop(struct casync_loop* loop)
{
    loop->return_code = 0;

    while (1)
    {
        /* Run our chain */
        struct casync_loop* store_loop = casync_current_loop;
        casync_current_loop = loop;
        assert(loop->active == &loop->control_task);
        casync_yield();
        casync_current_loop = store_loop;

        if (&loop->control_task == loop->control_task.next)
            break;

        /* Context switch to parent casync_gather(), if one exists */
        if (casync_current_loop)
            casync_yield();
    }

    return loop->return_code;
}

/* -------------------------------------------------------------------------- */
int casync_gather_static(struct casync_task* freelist, int n, ...)
{
    va_list            ap;
    struct casync_loop loop;

    loop.control_task.next = &loop.control_task;
    loop.active = &loop.control_task;
    loop.finished = freelist;

    va_start(ap, n);
    while (n--)
    {
        void* function = va_arg(ap, void*);
        void* arg = va_arg(ap, void*);
        loop_start_static(&loop, function, arg);
    }
    va_end(ap);

    return casync_run_loop(&loop);
}

/* -------------------------------------------------------------------------- */
int casync_gather(int n, ...)
{
    va_list             ap;
    struct casync_loop  loop;
    struct casync_task* t;
    struct casync_task* next;
    int                 rc;

    loop.control_task.next = &loop.control_task;
    loop.active = &loop.control_task;
    loop.finished = NULL;

    va_start(ap, n);
    while (n--)
    {
        void* function = va_arg(ap, void*);
        void* arg = va_arg(ap, void*);
        loop_start(&loop, function, arg);
    }
    va_end(ap);

    rc = casync_run_loop(&loop);

    for (t = loop.finished; t; t = next)
    {
        next = t->next;
        free(t);
    }

    return rc;
}

/* -------------------------------------------------------------------------- */
struct casync_task* casync_stack_pool_init_linear(
    void* stacks_memory, size_t stack_size, size_t stack_count)
{
    size_t              i;
    struct casync_task* freelist = NULL;
    for (i = 0; i != stack_count; ++i)
    {
        uint8_t*            offset = (uint8_t*)stacks_memory + stack_size * i;
        struct casync_task* t = (struct casync_task*)offset;
        t->stack_size = stack_size;
        t->next = freelist;
        freelist = t;
    }

    return freelist;
}
