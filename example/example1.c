#include "casync/casync.h"
#include <stdio.h>

static int task(void* arg) {
    fprintf(stderr, "task %d\n", (int)(intptr_t)arg);
    casync_yield();
    fprintf(stderr, "task %d\n", (int)(intptr_t)arg);
    return 0;
}

int main(void) {
    /* clang-format off */
    return casync_gather(2,
        task, (void*)1,
        task, (void*)2);
    /* clang-format on */
}
