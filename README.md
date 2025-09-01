# Simple Coroutines in C

I was inspired by [Coroutines in C by Simon Tatham](https://www.chiark.greenend.org.uk/~sgtatham/coroutines.html).

This library implements  a  very  simple  round-robin  scheduler  with a proper
context switching  mechanism  to  jump  between  co-routines. It is cooperative
multitasking in userspace.

The two main functions to do this are:
  + ```casync_gather()```
  + ```casync_yield()```

For example:

```c
#include "casync/casync.h"
#include <stdio.h>

static int task(void* arg) {
    fprintf(stderr, "task %d\n", (int)(intptr_t)arg);
    casync_yield();
    fprintf(stderr, "task %d\n", (int)(intptr_t)arg);
    return 0;
}

int main(void) {
    return casync_gather(2,
        task, (void*)1,
        task, (void*)2);
}
```

Prints:

```
task 1
task 2
task 1
task 2
```

As  you  can see, the the ```task()``` function is  executed  as  a  co-routine
twice,  and  execution  jumps   between   the   two  via  ```casync_yield()```.

# Features and Limitations

Limitations:
 + Currently, only the general purpose  registers  are  saved.  That means FPU and
   SIMD state will leak across co-routines. Adding  support  for  this is trivial,
   though, so expect an update soon.
 + The Windows port has not been done yet. This is coming soon as well.

Features:
 + The scheduler state is stored in TLS (thread-local storage), meaning, you can
   run co-routines in multiple threads at the same time. The usual synchronization
   primitives can  be  used  to  synchronize  data  across  threaded  co-routines.

# Static API

The "normal"  API uses ```malloc()``` internally to allocate the stack for each
co-routine. The default stack size is 1M.

If you want to supply  your  own memory to be used as stack space, then you can
use  the  ```_static()```  functions such as ```casync_gather_static()```.  For
example, if you  know  that  you  will  have  at  most  5  co-routines  running
simulatneously in a call to ```gather```, then you  can  define  the  memory up
front:

```c
 static size_t stacks[1024 * 64][5] __attribute__((aligned(16)));
 struct casync_task* freelist = casync_stack_pool_init_linear(stacks,
   sizeof(stacks) / sizeof(*stacks),
   sizeof(*stacks) / sizeof(**stacks));
 ```

The freelist can then be passed to ```gather```:

```c
casync_gather_static(freelist, 2,
    casync_gather(2,
        task, (void*)1,
        task, (void*)2);
```

# Starting co-routines dynamically

This use-case is  covered  in  ```example3.c``` where a server will start a new
co-routine  for  every  client  that  joins.  This  is  accomplished using  the
```casync_start()```   function.   ```casync_start()```  will  create   a   new
co-routine  and  add  it  to  the active list within the current  ``gather()```
context.  It will be as if you had called ```gather()```  with  the  co-routine
added there. For example:

```c
static int dynamic(void* arg)
{
    fprintf(stderr, "dynamic\n");
    return 0;
}

static int task(void* arg) {
    casync_start(dynamic, NULL);
    fprintf(stderr, "task %d\n", (int)(intptr_t)arg);
    casync_yield();
    fprintf(stderr, "task %d\n", (int)(intptr_t)arg);

    return 0;
}

int main(void) {
    casync_gather(2,
        task, (void*)1,
        task, (void*)2);
}
```

This will print:

```
task 1
task 2
dynamic
task 1
dynamic
task 2
```

As you can see, ```dynamic``` is printed twice. Why? Well, because ```task()```
is run twice, and each instance of  ```task()```  adds  ```dynamic```  as well.

# Error Handling

As  of now, error handling doesn't work as I intend yet. Co-routines  have  the
function signature ```int func(void*)```. The intention is to be able to return
0 for success and -1 for error. If any co-routine returns  an  error code, then
```casync_gather()``` should also return an error (doesn't work yet).
