
#include <malloc.h>
#include <assert.h>

#include "timing.h"

#include "lpel_p.h"


/**
 * Functions for task management
 */



/**
 * Create a task
 */
task_t *TaskCreate( void (*func)(void *arg), void *arg, unsigned int attr)
{
  task_t *t = (task_t *)malloc( sizeof(task_t) );
  t->state = TASK_INIT;
  
  t->prev = t->next = NULL;
  
  t->event_ptr = NULL;
  t->ev_write = t->ev_read = false;
  
  t->owner = LpelGetWorkerId();
  t->sched_info = NULL;

  TimingStart(&t->time_alive);
  /* zero all timings */
  TimingZero(&t->time_totalrun);
  t->time_lastrun = t->time_expavg = t->time_totalrun;

  t->cnt_dispatch = 0;
  StreamarrAlloc(&t->streams_writing, STREAMARR_INITSIZE);
  StreamarrAlloc(&t->streams_reading, STREAMARR_INITSIZE);

  t->code = co_create(func, arg, NULL, 8192); /* 8k stacksize */
  t->arg = arg;

  return t;
}


/**
 * Destroy a task
 */
void TaskDestroy(task_t *t)
{
  /* free inner members */
  StreamarrFree(&t->streams_writing);
  StreamarrFree(&t->streams_reading);
  co_delete(t->code);

  /* free the TCB itself*/
  free(t);
  t = NULL;
}


/**
 * Exit the current task
 * TODO joinarg
 */
void TaskExit(void)
{
  task_t *ct = LpelGetCurrentTask();
  assert( ct->state == TASK_RUNNING );

  ct->state = TASK_ZOMBIE;
  co_resume();

  /* execution never comes back here */
  assert(0);
}


/**
 * Yield execution back to worker thread
 */
void TaskYield(void)
{
  task_t *ct = LpelGetCurrentTask();
  assert( ct->state == TASK_RUNNING );

  ct->state = TASK_READY;
  co_resume();
}



/**
 * Join with a task
 */
/*TODO place in special queue which is notified each time a task dies */

