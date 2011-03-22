#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

#include "../../arch/timing.h"


#define NUM_THREADS 2
#define MAX_INC (200000000 / NUM_THREADS)

static volatile int counter;

static inline int fetch_and_dec(volatile int *cnt)
{
  int tmp = -1;
  asm volatile("lock; xadd%z0 %0,%1"
      : "=r" (tmp), "=m" (*cnt)
      : "0" (tmp), "m" (*cnt)
      : "memory", "cc");
  return tmp;
}

static inline int fetch_and_inc(volatile int *cnt)
{
  int tmp = 1;
  asm volatile("lock; xadd%z0 %0,%1"
      : "=r" (tmp), "=m" (*cnt)
      : "0" (tmp), "m" (*cnt)
      : "memory", "cc");
  return tmp;
}

static inline int atomic_dec(volatile int *cnt)
{
  unsigned char prev = 0;
  asm volatile ("lock; decl %0; setnz %1"
      : "=m" (*cnt), "=qm" (prev)
      : "m" (*cnt)
      : "memory", "cc");
  return (int)prev;
}


static inline void atomic_inc(volatile int *cnt)
{
  asm volatile ("lock; incl %0"
      : /* no output */
      : "m" (*cnt)
      : "memory", "cc");
}

void *thread_naive(void *arg)
{
  int i;
  for (i=0; i<MAX_INC; i++) counter += 1;
  pthread_exit(NULL);
}


void *thread_builtin(void *arg)
{
  int i;
  for (i=0; i<MAX_INC; i++) (void) __sync_fetch_and_add(&counter, 1);
  pthread_exit(NULL);
}


void *thread_asm(void *arg)
{
  int i;
  for (i=0; i<MAX_INC; i++) {
    atomic_inc(&counter);
  }
  pthread_exit(NULL);
}


void testIncToMax(void)
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

}



void testRefCnt(void)
{
  const int num = 10;
  int i,z;

  counter = 0;
  printf("Testing RefCnt...\n");
  for (i=0; i<num; i++) {
    atomic_inc(&counter);
  }
  for (i=0; i<num-1; i++) {
    z = atomic_dec(&counter);
    printf("counter %d z %d\n", counter, z);
    assert( z != 0);
  }
  /* reaching zero */
  z = atomic_dec(&counter);
  printf("counter %d z %d\n", counter, z);
  assert( z == 0);
  /* and one more */
  z = atomic_dec(&counter);
  printf("counter %d z %d\n", counter, z);
  assert( z != 0);
  printf("Success.\n");
}




int main(int argc, char **argv)
{
  testRefCnt();
  testIncToMax();
  return 0;
}
