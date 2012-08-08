#include <time.h>

#include "mutex.h"
#include "worker.h"
#include "lpel.h"

void LpelMutexInit(lpel_mutex_t *mx)
{
  __sync_lock_release(&mx->counter);
}

void LpelMutexDestroy(lpel_mutex_t *mx)
{
  /*NOP*/
}


void LpelMutexEnter(lpel_task_t *t, lpel_mutex_t *mx)
{
  while (1==__sync_lock_test_and_set(&mx->counter,1)) {
    /* mutex is busy, schedule another thread */
    t->state = TASK_READY;
#ifdef WAITING
    t->last_measurement_start = clock_gettime();
#endif
    t->last_measurement_start
    LpelWorkerSelfTaskYield(t);
    LpelTaskBlock(t);
  }
}

void LpelMutexLeave(lpel_task_t *t, lpel_mutex_t *mx)
{
  __sync_lock_release(&mx->counter);
}
