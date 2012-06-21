
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

#include "scheduler.h"


#include "taskqueue.h"
#include "task.h"
#include "taskiterator.h"

struct schedctx {
  pthread_mutex_t queue_mutex;
  taskqueue_t queue[SCHED_NUM_PRIO];
};


schedctx_t *LpelSchedCreate( int wid)
{
  int i;
  schedctx_t *sc = (schedctx_t *) malloc( sizeof(schedctx_t));
  for (i=0; i<SCHED_NUM_PRIO; i++) {
    LpelTaskqueueInit( &sc->queue[i]);
  }
  pthread_mutex_init(&sc->queue_mutex, NULL);
  return sc;
}


void LpelSchedDestroy( schedctx_t *sc)
{
  int i;
  for (i=0; i<SCHED_NUM_PRIO; i++) {
    assert( sc->queue[i].count == 0);
  }
  pthread_mutex_destroy(&sc->queue_mutex);
  free( sc);
}



void LpelSchedMakeReady( schedctx_t* sc, lpel_task_t *t)
{
  int prio = t->sched_info.prio;

  if (prio < 0) prio = 0;
  if (prio >= SCHED_NUM_PRIO) prio = SCHED_NUM_PRIO-1;
  pthread_mutex_lock(&sc->queue_mutex);
  LpelTaskqueuePushBack( &sc->queue[prio], t);
  pthread_mutex_unlock(&sc->queue_mutex);
}


lpel_task_t *LpelSchedFetchReady( schedctx_t *sc)
{
  lpel_task_t *t = NULL;
  int i;
  pthread_mutex_lock(&sc->queue_mutex);
  for (i=SCHED_NUM_PRIO-1; i>=0; i--) {
    if (sc->queue[i].count > 0) {
      t = LpelTaskqueuePopFront( &sc->queue[i]);
      break;
    }
  }
  pthread_mutex_unlock(&sc->queue_mutex);

  return t;
}

void LpelSchedTaskIter( schedctx_t *sc)
{
  return LpelTaskIterReset( sc->queue, SCHED_NUM_PRIO);
}

void LpelSchedLockQueue( schedctx_t *sc)
{
  pthread_mutex_lock(&sc->queue_mutex);
}
void LpelSchedUnlockQueue( schedctx_t *sc)
{
  pthread_mutex_unlock(&sc->queue_mutex);
}
