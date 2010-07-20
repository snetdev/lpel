#include <stdlib.h>

#include "scheduler.h"

#include "taskqueue.h"

struct readyset {
  taskqueue_t q;
};



/**
 * return an initialised readyset
 */
readyset_t *SchedInit(void)
{
  return (readyset_t *) calloc( 1, sizeof(readyset_t) );
}

void SchedCleanup(readyset_t *ready)
{
  free(ready);
  ready = NULL;
}

void SchedPutReady(readyset_t *ready, task_t *t)
{
  TaskqueueAppend( &ready->q, t );
}

task_t *SchedFetchNextReady(readyset_t *ready)
{
  return TaskqueueRemove( &ready->q );
}
