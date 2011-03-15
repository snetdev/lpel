
#include <stdlib.h>
#include <stdio.h>

#include <malloc.h>
#include <unistd.h>

#include <pcl.h>


typedef struct {
  int dummy;
  coroutine_t coro;
  void* stack;
} tcb_t;

static tcb_t *tcb;


void coro_func(void *arg)
{
  int a;
  printf("Address of a: %p\n", &a);
  printf("Difference tcb-a: %ld\n", (long)tcb-(long)&a);
  co_resume();
}

void main(int argc, char **argv)
{
  int pagesize = sysconf(_SC_PAGESIZE);
  int stacksize = 4*pagesize;

  void *mem = memalign(pagesize, stacksize);
  
  /* following is unsafe, as*/
  tcb = mem+stacksize-sizeof(tcb_t);

  printf("Test exec stack\n");

  printf("Pagesize: %d\n", pagesize);
  printf("Memory ptr: %p\n", mem);

  printf("Memory start of tcb: %p\n", (void *) tcb);


  printf("Size of TCB: %ld\n", sizeof(tcb_t));

  tcb->coro = co_create(coro_func, NULL, mem, stacksize-sizeof(tcb_t));
  co_call(tcb->coro);


  printf("Test finished\n");
}

