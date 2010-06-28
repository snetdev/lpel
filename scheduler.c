#include "scheduler.h"

#include "taskqueue.h"

static taskqueue_t queue;

/**
 * returns a pointer for the sched_info
 */
void *SchedInit(void)
{
  return &queue;
}

void SchedPutReady(void *ready, task_t *t)
{
  TaskqueueAppend( (taskqueue_t *) ready, t );
}

task_t *SchedFetchNextReady(void *ready)
{
  return TaskqueueRemove( (taskqueue_t *) ready );
}
