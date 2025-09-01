#include "casync/casync.h"
#include <time.h>
#include <stdint.h>

static uint64_t ts_to_ns(struct timespec ts)
{
    return (uint64_t)ts.tv_sec * 1000000000 + (uint64_t)ts.tv_nsec;
}

void casync_sleep_ns(uint64_t ns)
{
    uint64_t        now, wait_until;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    wait_until = ts_to_ns(ts) + ns;

    do
    {
        casync_yield();
        clock_gettime(CLOCK_MONOTONIC, &ts);
        now = ts_to_ns(ts);
    } while (now < wait_until);
}

