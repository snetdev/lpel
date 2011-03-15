
#include <stdlib.h>
#include <stdio.h>

#include <malloc.h>
#include <unistd.h>

#include <pcl.h>

static long stackend;

typedef struct {
  int dummy;
  coroutine_t coro;
  void* stack;
} tcb_t;

void coro_func(void *arg)
{
  int a;
  printf("Address of a: %p\n", &a);
  printf("Difference stackend-a: %ld\n", stackend-(long)&a);
  co_resume();
}

void main(int argc, char **argv)
{
  int pagesize = sysconf(_SC_PAGESIZE);
  int stacksize = 4*pagesize;
  int offset = sizeof(tcb_t);
  tcb_t *tcb;

  void *mem = memalign(pagesize, stacksize);
  
  tcb = mem;

  printf("Test exec stack\n");

  printf("Pagesize: %d\n", pagesize);
  printf("Memory ptr: %p\n", mem);

  stackend = (long) mem+stacksize;
  printf("Memory ptr+stacksize: %p\n", (void *) stackend);


  printf("Size of TCB: %d\n", offset);
  tcb->coro = co_create(coro_func, NULL, mem+offset, stacksize-offset);
  co_call(tcb->coro);


  printf("Test finished\n");
}

