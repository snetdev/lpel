#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

#include "../../timing.h"


#define NUM_THREADS 2
#define MAX_INC (200000000 / NUM_THREADS)

static int counter;

static inline int fetch_and_inc(volatile int *cnt)
{
  int tmp = 1;
  asm volatile("lock\n\t" "xadd%z0 %0,%1"
             : "=r" (tmp), "=m" (*cnt)
             : "0" (tmp), "m" (*cnt)
             : "memory", "cc");
  return tmp;
}

void *thread_naive(void *arg)
{
  int i;
  for (i=0; i<MAX_INC; i++) counter += 1;
  pthread_exit(NULL);
}


void *thread_builtin(void *arg)
{
  int i, z;
  for (i=0; i<MAX_INC; i++) z = __sync_fetch_and_add(&counter, 1);
  pthread_exit(NULL);
}


void *thread_asm(void *arg)
{
  int i,z;
  for (i=0; i<MAX_INC; i++) {
    z = fetch_and_inc(&counter);
#if NUM_THREADS == 1
    assert(z==i);
#endif
  }
  pthread_exit(NULL);
}





int main(int argc, char **argv)
{
  int n;
  pthread_t th[NUM_THREADS];
  double msec;
  timing_t tt;

  printf("Atomic counter test, incrementing %d times.\n", MAX_INC);

  counter = 0;
  printf("Testing naive...\n");
  TimingStart(&tt);
  for (n=0; n<NUM_THREADS; n++) {
    (void) pthread_create(&th[n], NULL, thread_naive, NULL);  
  }
  for (n=0; n<NUM_THREADS; n++) {
    (void) pthread_join(th[n], NULL);  
  }
  TimingEnd(&tt);
  msec = TimingToMSec(&tt);
  printf("Result: %d, in %f msec.\n", counter, msec);

  counter = 0;
  printf("Testing builtin...\n");
  TimingStart(&tt);
  for (n=0; n<NUM_THREADS; n++) {
    (void) pthread_create(&th[n], NULL, thread_builtin, NULL);  
  }
  for (n=0; n<NUM_THREADS; n++) {
    (void) pthread_join(th[n], NULL);  
  }
  TimingEnd(&tt);
  msec = TimingToMSec(&tt);
  printf("Result: %d, in %f msec.\n", counter, msec);


  counter = 0;
  printf("Testing inline asm...\n");
  TimingStart(&tt);
  for (n=0; n<NUM_THREADS; n++) {
    (void) pthread_create(&th[n], NULL, thread_asm, NULL);  
  }
  for (n=0; n<NUM_THREADS; n++) {
    (void) pthread_join(th[n], NULL);  
  }
  TimingEnd(&tt);
  msec = TimingToMSec(&tt);
  printf("Result: %d, in %f msec.\n", counter, msec);

  return 0;
}
