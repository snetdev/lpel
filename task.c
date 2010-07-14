
#include <malloc.h>
#include <assert.h>

#include "task.h"

#include "atomic.h"
#include "timing.h"
#include "lpel.h"

#include "debug.h"

static sequencer_t taskseq = SEQUENCER_INIT;


/**
 * Create a task
 */
task_t *TaskCreate( void (*func)(void *), void *arg, unsigned int attr)
{
  task_t *t = (task_t *)malloc( sizeof(task_t) );
  t->uid = ticket(&taskseq);

  t->state = TASK_INIT;
  
  t->prev = t->next = NULL;

  /* initialize reference counter to 1*/
  atomic_set(&t->refcnt, 1);

  t->event_ptr = NULL;
  t->ev_write = t->ev_read = false;
  
  //t->owner = -1;
  t->sched_info = NULL;

  TimingStart(&t->time_alive);
  /* zero all timings */
  TimingZero(&t->time_totalrun);
  t->time_lastrun = t->time_expavg = t->time_totalrun;

  t->cnt_dispatch = 0;
  SetAlloc(&t->streams_writing);
  SetAlloc(&t->streams_reading);

  t->code = co_create(func, arg, NULL, TASK_STACKSIZE);
  if (t->code == NULL) {
    /*TODO throw error!*/
  }
  t->arg = arg;
 
  /* Notify LPEL */
  LpelTaskAdd(t);

  return t;
}


/**
 * Destroy a task
 */
void TaskDestroy(task_t *t)
{
  /* only if no stream points to the flags anymore */
  if ( fetch_and_dec(&t->refcnt) == 1) {
    DBG("free task %lu", t->uid);

    /* Notify LPEL first */
    LpelTaskRemove(t);

    /* free inner members */
    SetFree(&t->streams_writing);
    SetFree(&t->streams_reading);
    if (t->code != NULL)  co_delete(t->code);

    /* free the TCB itself*/
    free(t);
    t = NULL;
  }
}


void TaskWaitOnRead(void)
{
  task_t *ct = LpelGetCurrentTask();
  /* WAIT on read event*/;
  ct->event_ptr = &ct->ev_read;
  ct->state = TASK_WAITING;
  DBG("task %lu waits on read", ct->uid);
  /* context switch */
  co_resume();
}

void TaskWaitOnWrite(void)
{
  task_t *ct = LpelGetCurrentTask();
  /* WAIT on write event*/
  ct->event_ptr = &ct->ev_write;
  ct->state = TASK_WAITING;
  DBG("task %lu waits on write", ct->uid);
  /* context switch */
  co_resume();
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

