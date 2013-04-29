
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

#include "decen_scheduler.h"


#include "decen_taskqueue.h"
#include "task.h"
#include "task_migration.h"


struct schedctx_t {
  taskqueue_t *queue[SCHED_NUM_PRIO];
};


schedctx_t *LpelSchedCreate( int wid)
{
  int i;
  schedctx_t *sc = (schedctx_t *) malloc( sizeof(schedctx_t));
  for (i=0; i<SCHED_NUM_PRIO; i++) {
    sc->queue[i] = LpelTaskqueueInit();
  }
  return sc;
}


void LpelSchedDestroy( schedctx_t *sc)
{
  int i;
  for (i=0; i<SCHED_NUM_PRIO; i++) {
    assert( sc->queue[i]->count == 0);
    LpelTaskqueueDestroy(sc->queue[i]);
  }

  free( sc);
}



void LpelSchedMakeReady( schedctx_t* sc, lpel_task_t *t)
{
  int prio = t->sched_info->prio;

  if (prio < 0) prio = 0;
  if (prio >= SCHED_NUM_PRIO) prio = SCHED_NUM_PRIO-1;
  LpelTaskqueuePush( sc->queue[prio], t);
}


lpel_task_t *LpelSchedFetchReady( schedctx_t *sc)
{
  lpel_task_t *t = NULL;
  int i;
  for (i=SCHED_NUM_PRIO-1; i>=0; i--) {
    if (sc->queue[i]->count > 0) {
      t = LpelTaskqueuePop( sc->queue[i]);
      break;
    }
  }

  return t;
}
