
#include <stdlib.h>
#include <assert.h>

#include "arch/atomic.h"

#include "task.h"

#include "worker.h"
#include "stream.h"

#include "monitoring.h"



static atomic_t taskseq = ATOMIC_INIT(0);



/* declaration of startup function */
//static void TaskStartup( unsigned int y, unsigned int x);
static void TaskStartup( void *arg);

static void TaskStart( lpel_task_t *t);
static void TaskStop( lpel_task_t *t);
static void TaskBlock( lpel_task_t *t, taskstate_t state);


#define TASK_STACK_ALIGN  256
#define TASK_MINSIZE  4096


/**
 * Create a task.
 *
 * @param worker  id of the worker where to create the task
 * @param func    task function
 * @param arg     arguments
 * @param size    size of the task, including execution stack
 * @pre           size is a power of two, >= 4096
 *
 * @return the task handle of the created task (pointer to TCB)
 *
 * TODO reuse task contexts from the worker
 */
lpel_task_t *LpelTaskCreate( int worker, lpel_taskfunc_t func,
    void *inarg, int size)
{
  lpel_task_t *t;
  char *stackaddr;
  int offset;

  if (size <= 0) {
    size = LPEL_TASK_SIZE_DEFAULT;
  }
  assert( size >= TASK_MINSIZE );

  /* aligned at page boundary */
  t = valloc( size );

  /* calc stackaddr */
  offset = (sizeof(lpel_task_t) + TASK_STACK_ALIGN-1) & ~(TASK_STACK_ALIGN-1);
  stackaddr = (char *) t + offset;
  t->size = size;


  /* obtain a usable worker context */
  t->worker_context = LpelWorkerGetContext(worker);


  t->uid = fetch_and_inc( &taskseq);  /* obtain a unique task id */
  t->func = func;
  t->inarg = inarg;

  /* initialize poll token to 0 */
  atomic_init( &t->poll_token, 0);

  t->state = TASK_CREATED;

  t->prev = t->next = NULL;

  t->mon = NULL;

  mctx_create( &t->mctx, TaskStartup, (void*)t, stackaddr, t->size - offset);
  // if (t->mctx == NULL) assert(0);
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

  free(t);
}


/**
 * Attach monitoring to a task
 */
void LpelTaskMonitor( lpel_task_t *t, const char *name, unsigned long flags)
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
 * Get the current task
 *
 * @pre This call must be made from within a LPEL task!
 */
lpel_task_t *LpelTaskSelf(void)
{
  return LpelWorkerCurrentTask();
}


/**
 * Exit the current task
 *
 * @param outarg  output argument of the task
 * @pre This call must be made from within a LPEL task!
 */
void LpelTaskExit(void *outarg)
{
  lpel_task_t *ct = LpelTaskSelf();
  assert( ct->state == TASK_RUNNING );

  ct->outarg = outarg;

  /* context switch happens, this task is cleaned up then */
  TaskBlock( ct, TASK_ZOMBIE);
  /* execution never comes back here */
  assert(0);
}


/**
 * Yield execution back to scheduler voluntarily
 *
 * @pre This call must be made from within a LPEL task!
 */
void LpelTaskYield(void)
{
  lpel_task_t *ct = LpelTaskSelf();
  assert( ct->state == TASK_RUNNING );

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
 */
//static void TaskStartup( unsigned int y, unsigned int x)
static void TaskStartup( void *arg)
{
  lpel_task_t *t;

  t = (lpel_task_t *)arg;
#if 0
  unsigned long z;

  z = x<<16;
  z <<= 16;
  z |= y;
  t = (lpel_task_t *)z;
#endif
  TaskStart( t);

  /* call the task function with inarg as parameter */
  t->outarg = t->func(t->inarg);

  /* if task function returns, exit properly */
  TaskBlock( t, TASK_ZOMBIE);
  /* execution never comes back here */
  assert(0);
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


