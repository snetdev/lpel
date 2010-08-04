
#include <malloc.h>
#include <assert.h>

#include "task.h"

#include "atomic.h"
#include "timing.h"
#include "lpel.h"

#include "debug.h"

static sequencer_t taskseq = SEQUENCER_INIT;



/* declaration of startup function */
static void TaskStartup(void *data);


/**
 * Create a task
 */
task_t *TaskCreate( taskfunc_t func, void *inarg, unsigned int attr)
{
  task_t *t = (task_t *)malloc( sizeof(task_t) );
  t->uid = ticket(&taskseq);

  t->state = TASK_INIT;
  
  t->prev = t->next = NULL;

  /* initialize reference counter to 1*/
  atomic_set(&t->refcnt, 1);

  t->event_ptr = NULL;
  t->ev_write = t->ev_read = 0;
  
  //t->owner = -1;
  t->sched_info = NULL;

  TimingStart(&t->time_alive);
  /* zero all timings */
  TimingZero(&t->time_totalrun);
  t->time_lastrun = t->time_expavg = t->time_totalrun;

  t->cnt_dispatch = 0;
  t->streamtab = NULL;

  t->code = func;
  /* function, argument (data), stack base address, stacksize */
  t->ctx = co_create(TaskStartup, (void *)t, NULL, TASK_STACKSIZE);
  if (t->ctx == NULL) {
    /*TODO throw error!*/
  }
  t->inarg = inarg;
  t->outarg = NULL;
 
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

    /* is the streamtable empty? */
    StreamtableClean(&t->streamtab);
    assert( t->streamtab == NULL );

    /* delete the coroutine */
    co_delete(t->ctx);

    /* free the TCB itself*/
    free(t);
    t = NULL;
  }
}


/**
 * Hidden Startup function for user specified task function
 *
 * Calls task function with proper signature
 */
static void TaskStartup(void *data)
{
  task_t *t = (task_t *)data;
  taskfunc_t func = t->code;
  /* call the task function with inarg as parameter */
  func(t, t->inarg);

  /* if task function returns, exit properly */
  TaskExit(t, NULL);
}



/**
 * Set Task waiting for a read event
 *
 * @param ct  pointer to the current task
 */
void TaskWaitOnRead(task_t *ct)
{
  /* WAIT on read event*/;
  ct->event_ptr = &ct->ev_read;
  ct->state = TASK_WAITING;
  DBG("task %lu waits on read", ct->uid);
  /* context switch */
  co_resume();
}

/**
 * Set Task waiting for a read event
 *
 * @param ct  pointer to the current task
 */
void TaskWaitOnWrite(task_t *ct)
{
  /* WAIT on write event*/
  ct->event_ptr = &ct->ev_write;
  ct->state = TASK_WAITING;
  DBG("task %lu waits on write", ct->uid);
  /* context switch */
  co_resume();
}

/**
 * Exit the current task
 *
 * @param ct  pointer to the current task
 * @param outarg  join argument
 */
void TaskExit(task_t *ct, void *outarg)
{
  assert( ct->state == TASK_RUNNING );

  ct->state = TASK_ZOMBIE;
  ct->outarg = outarg;
  co_resume();

  /* execution never comes back here */
  assert(0);
}


/**
 * Yield execution back to worker thread
 *
 * @param ct  pointer to the current task
 */
void TaskYield(task_t *ct)
{
  assert( ct->state == TASK_RUNNING );

  ct->state = TASK_READY;
  co_resume();
}



/**
 * Join with a task
 */
/*TODO place in special queue which is notified each time a task dies */

