
#include <stdlib.h>
#include <assert.h>

#include "arch/atomic.h"
#include "arch/timing.h"

#include "task.h"

#include "worker.h"
#include "stream.h"

#include "monitoring.h"



static atomic_t taskseq = ATOMIC_INIT(0);



/* declaration of startup function */
static void TaskStartup(void *data);

static void TaskStart( lpel_task_t *t);
static void TaskStop( lpel_task_t *t);
static void TaskBlock( lpel_task_t *t, taskstate_t state);





/**
 * Create a task.
 *
 * @param worker  id of the worker where to create the task
 * @param func    task function
 * @param arg     arguments
 * @param stacksize   size of the execution stack of the task
 *
 * @return the task handle of the created task (pointer to TCB)
 */
lpel_task_t *LpelTaskCreate( int worker, lpel_taskfunc_t func,
    void *inarg, int stacksize )
{
  /* obtain a task context */
  /*TODO reuse one from the worker */
  lpel_task_t *t = malloc( sizeof(lpel_task_t));
  

  /* obtain a usable worker context */
  t->worker_context = LpelWorkerGetContext(worker);


  t->uid = fetch_and_inc( &taskseq);  /* obtain a unique task id */
  t->func = func;
  t->inarg = inarg;

  t->stacksize = stacksize;
  if (t->stacksize <= 0) {
    t->stacksize = LPEL_TASK_ATTR_STACKSIZE_DEFAULT;
  }

  /* initialize poll token to 0 */
  atomic_init( &t->poll_token, 0);

  t->state = TASK_CREATED;

  t->prev = t->next = NULL;

  t->mon = NULL;
  
  /* function, argument (data), stack base address, stacksize */
  t->mctx = co_create( TaskStartup, (void *)t, NULL, t->stacksize);
  if (t->mctx == NULL) {
    /*TODO throw error!*/
    assert(0);
  }
  return t;
}



/**
 * Destroy a task
 * - completely free the memory for that task
 */
void LpelTaskDestroy( lpel_task_t *t)
{
  assert( t->state == TASK_ZOMBIE);

  /* if task had a monitoring object, destroy it */
  if (t->mon) LpelMonTaskDestroy(t->mon);

  atomic_destroy( &t->poll_token);
  /* delete the coroutine */
  co_delete(t->mctx);
  /* free the TCB itself*/
  free(t);
}


/**
 * Attach monitoring to a task
 */
void LpelTaskMonitor( lpel_task_t *t, char *name, unsigned long flags)
{
  t->mon = LpelMonTaskCreate(t->uid, name, flags);
}




/**
 * Let the task run on the worker
 */
void LpelTaskRun( lpel_task_t *t)
{
  assert( t->state == TASK_CREATED );

  LpelWorkerRunTask( t);
}



/**
 * Exit the current task
 *
 * @param ct  pointer to the current task
 * @pre ct->state == TASK_RUNNING
 */
void LpelTaskExit( lpel_task_t *ct)
{
  assert( ct->state == TASK_RUNNING );

  /* context switch happens, this task is cleaned up then */
  TaskBlock( ct, TASK_ZOMBIE);
  /* execution never comes back here */
  assert(0);
}


/**
 * Yield execution back to scheduler voluntarily
 *
 * @param ct  pointer to the current task
 * @pre ct->state == TASK_RUNNING
 */
void LpelTaskYield( lpel_task_t *ct)
{
  TaskBlock( ct, TASK_READY);
}


unsigned int LpelTaskGetUID( lpel_task_t *t)
{
  return t->uid;
}




/**
 * Block a task
 */
void LpelTaskBlock(lpel_task_t *ct, taskstate_blocked_t block_on)
{
  ct->blocked_on = block_on;
  TaskBlock( ct, TASK_BLOCKED);
}


/**
 * Unblock a task. Called from StreamRead/StreamWrite procedures
 */
void LpelTaskUnblock( lpel_task_t *ct, lpel_task_t *blocked)
{
  assert(ct != NULL);
  assert(blocked != NULL);

  LpelWorkerTaskWakeup( ct, blocked);
}


/******************************************************************************/
/* PRIVATE FUNCTIONS                                                          */
/******************************************************************************/

/**
 * Startup function for user specified task,
 * calls task function with proper signature
 *
 * @param data  the previously allocated lpel_task_t TCB
 */
static void TaskStartup( void *data)
{
  lpel_task_t *t = (lpel_task_t *)data;

  TaskStart( t);

  /* call the task function with inarg as parameter */
  t->func(t, t->inarg);
  /* if task function returns, exit properly */
  LpelTaskExit(t);
}


static void TaskStart( lpel_task_t *t)
{
  assert( t->state == TASK_READY );

  /* MONITORING CALLBACK */
  if (t->mon) LpelMonTaskStart(t->mon);

  t->state = TASK_RUNNING;    
}

static void TaskStop( lpel_task_t *t)
{
  //workerctx_t *wc = t->worker_context;
  assert( t->state != TASK_RUNNING);

  /* MONITORING CALLBACK */
  if (t->mon) LpelMonTaskStop(t->mon, t->state);
}


static void TaskBlock( lpel_task_t *t, taskstate_t state)
{

  assert( t->state == TASK_RUNNING);
  assert( state == TASK_READY || state == TASK_ZOMBIE || state == TASK_BLOCKED);

  /* set new state */
  t->state = state;

  TaskStop( t);
  LpelWorkerDispatcher( t);
  TaskStart( t);
}


