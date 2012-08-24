
#include "task.h"
#include "worker.h"
#include <sys/time.h>


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
  struct timeval start_tv, current_tv;
  int looping = 0;

  /* __sync_lock_test_and_set is an atomic exchange.
   *  It writes value into *ptr, and returns the previous contents of *ptr.
   * Hence, the while loop is executed as long as the previous value in the
   * counter has been 1 (non-signalled).
   */
  while (1 == __sync_lock_test_and_set(&sem->counter,1)) {
    /* Throttle LPEL task re-scheduling to max 1000 times per sec.
     * Rationale: Binary semaphores are used in SAC to implement fast barriers.
     * Ideally there shouldn't be other tasks on those workers that run SAC bees.
     * The need for task rescheduling on these dedicated workers is very low.
     * Therefore, to optimize overheads this function will try to re-schedule
     * the LPEL worker at most once per 1 milisecond.
     */
    if (!looping) {
      /* the first cycle */
      gettimeofday(&start_tv, NULL);
      looping = 1;
    } else {
      /* not the first cycle; determine the current time */
      gettimeofday(&current_tv, NULL);    /* this takes roughly hundreds of nanoseconds */
      /* has elapsed more than, say, roughly 1 ms? */
      int yeah = 0;
      long sec_diff = current_tv.tv_sec - start_tv.tv_sec;
      if (sec_diff != 0) {
        yeah = 1;
      } else {
        long usec_diff = current_tv.tv_usec - start_tv.tv_usec;
        if (usec_diff > 1000) {
          yeah = 1;
        }
      }

      if (yeah) {
        start_tv = current_tv;
        /* re-schedule another task */
        lpel_task_t *t = LpelTaskSelf();
        t->state = TASK_READY;
        LpelWorkerSelfTaskYield(t);
        LpelTaskBlock(t);
      }
    }
  }
}

/** Signal the semaphore, possibly releasing a waiting task. */
void LpelBiSemaSignal(lpel_bisema_t *sem)
{
  /* This simply writes 0 */
  __sync_lock_release(&sem->counter);
}
