#pragma once

#include <stddef.h>
#include <stdint.h>

struct casync_task;

/*!
 * @brief Transfers control to another co-routine. This is typically called
 * when waiting on an I/O operation. For example:
 *
 *   ```c
 *   while ((rc = recv(...)) == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
 *       casync_yield();
 *   ```
 *
 * @note This must be called within an casync_gather(), otherwise it will crash.
 */
extern void casync_yield(void);

/*!
 * @brief Runs a set of co-routines until all complete.
 *
 * In the _static version, each co-routine allocates its stack from a pool of
 * static memory. You can create a freelist from a pool of memory with @see
 * casync_stack_pool_init_linear. In the normal version, the memory is
 * allocated using malloc() and freed before the function returns.
 *
 * @param[in] n The number of co-routines to gather. Each co-routine expects a
 * function pointer followed by a user-pointer, which is passed to the
 * co-routine function as an argument. Example:
 *
 *   ```c
 *   struct arg arg1 = {...};
 *   struct arg arg2 = {...};
 *   casync_gather_static(freelist,
 *     2,
 *     my_coroutine_1, &arg1,
 *     my_coroutine_2, &arg2);
 *  ```
 *
 * When  nesting  multiple  ```casync_gather()``` calls, co-routines  in outer
 * gathers are still executed. This is by design.
 */
int casync_gather_static(struct casync_task* freelist, int n, ...);
int casync_gather(int n, ...);

/*!
 * @brief Starts a new co-routine that will run in parallel with the rest of
 * the currently active co-routines within the current async_gather() context.
 *
 * In the _static version, the new co-routine allocates its stack from the pool
 * of static memory that was passed to casync_gather_static(). In the normal
 * version, the memory is allocated using malloc() and freed before the
 * function returns.
 */
void casync_start_static(int (*function)(void*), void* arg);
void casync_start(int (*function)(void*), void* arg);

/*!
 * @brief Initialize stack memory to be used with casync_gather_static(). For
 * example, if you want to create a pool of 5 stacks, each with 1024*64 words of
 * memory:
 *
 *   ```c
 *   static size_t stacks[1024 * 64][5];
 *   freelist = casync_stack_pool_init_linear(stacks,
 *     sizeof(stacks) / sizeof(*stacks),
 *     sizeof(*stacks) / sizeof(**stacks));
 *   ```
 */
struct casync_task* casync_stack_pool_init_linear(
    void* stacks_memory, size_t stack_size, size_t stack_count);

/*!
 * @brief Yields until the specified amount of time has passed.
 * @note Contrary to the name, no sleeping is actually performed in the current
 * implementation. The function yields immediately until the time is passed.
 */
void casync_sleep_ns(uint64_t ns);
#define casync_sleep_ms(ms) casync_sleep_ns((ms) * 1000000)

/*!
 * @brief Internal function that is implemented differently depending on
 * platform/architecture.
 */
void* casync_init_stack(
    void* function,
    void* arg,
    void* return_addr,
    void* stack_buffer,
    int   stack_size);
