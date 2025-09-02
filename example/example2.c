#include "casync/casync.h"

#include <stdio.h>

#if defined(_MSC_VER)
#   define ALIGNED(x)
#else
#   define ALIGNED(x) __attribute__((aligned(x)))
#endif

static size_t task_stacks1[1024 * 64][2] ALIGNED(16);
static size_t task_stacks2[1024 * 64][2] ALIGNED(16);
static size_t runner_stacks[1024 * 64][2] ALIGNED(16);

static void task(void* arg)
{
    int i;
    for (i = 0; i != 5; ++i)
    {
        fprintf(stderr, "task %d: %d\n", (int)(intptr_t)arg, i);
        casync_yield();
    }
}

static void runner(void* arg)
{
    int   runner_idx = (int)(intptr_t)arg;
    void* stack = runner_idx == 1 ? task_stacks1 : task_stacks2;

    /* clang-format off */
    fprintf(stderr, "runner %d\n", runner_idx);
    casync_gather_static(
        casync_stack_pool_init_linear(stack,
            sizeof(task_stacks1)/sizeof(*task_stacks1),
            sizeof(*task_stacks1)/sizeof(**task_stacks1)),
        2,
        task, (void*)(intptr_t)(runner_idx * 2 + 0),
        task, (void*)(intptr_t)(runner_idx * 2 + 1));
    fprintf(stderr, "finished runner %d\n", runner_idx);
    /* clang-format on */
}

int main(void)
{
    /* clang-format off */
    casync_gather_static(
        casync_stack_pool_init_linear(runner_stacks,
            sizeof(runner_stacks)/sizeof(*runner_stacks),
            sizeof(*runner_stacks)/sizeof(**runner_stacks)),
        2,
        runner, (void*)1,
        runner, (void*)2);
    /* clang-format on */

    return 0;
}
