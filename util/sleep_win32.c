#include "casync/casync.h"

#define WIN32_LEAN_AND_MEAN
#include "windows.h"

void casync_sleep_ns(uint64_t ns)
{
    uint64_t now, wait_until;
    LARGE_INTEGER freq, ticks;
    QueryPerformanceCounter(&ticks);
    QueryPerformanceFrequency(&freq);
    now = ticks.QuadPart;
    wait_until = now + ns / freq.QuadPart;

    do
    {
        casync_yield();
        QueryPerformanceCounter(&ticks);
        now = ticks.QuadPart;
    } while (now < wait_until);
}
