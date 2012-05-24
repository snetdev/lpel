
#include "task.h"
#include "worker.h"


/** Initialize a binary semaphore. It is signalled by default. */
void LpelBiSemaInit(lpel_bisema_t *sem)
{
  __sync_lock_release(&sem->counter);
}

/** Destroy a semaphore */
void LpelBiSemaDestroy(lpel_bisema_t *sem)
{
  /*NOP*/
}

/** Wait on the semaphore */
void LpelBiSemaWait(lpel_bisema_t *sem)
{
  while (1 == __sync_lock_test_and_set(&sem->counter,1)) {
    /* mutex is busy, schedule another thread */
    lpel_task_t *t = LpelTaskSelf();
    t->state = TASK_READY;
    LpelWorkerSelfTaskYield(t);
    LpelTaskBlock(t);
  }
}

/** Signal the semaphore, possibly releasing a waiting task. */
void LpelBiSemaSignal(lpel_bisema_t *sem)
{
  /* This simply writes 0 */
  __sync_lock_release(&sem->counter);
}

/** Return the number of currently waiting tasks on the semaphore */
int LpelBiSemaCountWaiting(lpel_bisema_t *sem)
{
  return sem->counter;
}
