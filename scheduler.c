#include <stdlib.h>

#include "scheduler.h"

#include "taskqueue.h"


/**
 * returns a pointer for the sched_info
 */
void *SchedInit(void)
{
  return calloc( 1, sizeof(taskqueue_t) );
}

void SchedCleanup(void *ready)
{
  free(ready);
  ready = NULL;
}

void SchedPutReady(void *ready, task_t *t)
{
  TaskqueueAppend( (taskqueue_t *) ready, t );
}

task_t *SchedFetchNextReady(void *ready)
{
  return TaskqueueRemove( (taskqueue_t *) ready );
}
