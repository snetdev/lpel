
#include <stdlib.h>
#include <assert.h>

#include "arch/atomic.h"
#include "arch/timing.h"

#include "lpel.h"
#include "task.h"

#include "worker.h"
#include "stream.h"

#include "monitoring.h"


/**
 * Either use a pthread mutex or a pthread spinlock
 */
#ifdef TASK_USE_SPINLOCK
//#define TASKLOCK_TYPE       pthread_spinlock_t
#define TASKLOCK_INIT(x)    pthread_spin_init(x, PTHREAD_PROCESS_PRIVATE)
#define TASKLOCK_DESTROY(x) pthread_spin_destroy(x)
#define TASKLOCK_LOCK(x)    pthread_spin_lock(x)
#define TASKLOCK_UNLOCK(x)  pthread_spin_unlock(x)
#else
//#define TASKLOCK_TYPE       pthread_mutex_t
#define TASKLOCK_INIT(x)    pthread_mutex_init(x, NULL)
#define TASKLOCK_DESTROY(x) pthread_mutex_destroy(x)
#define TASKLOCK_LOCK(x)    pthread_mutex_lock(x)
#define TASKLOCK_UNLOCK(x)  pthread_mutex_unlock(x)

#endif /* TASK_USE_SPINLOCK */

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
 */
lpel_task_t *LpelTaskCreate( int worker, lpel_taskfunc_t func,
    void *inarg, int stacksize )
{
  /* obtain a task context */
  /*TODO reuse one from the worker */
  lpel_task_t *t = malloc( sizeof(lpel_task_t));
  
  t->worker_context = _LpelWorkerId2Wc(worker);

  t->uid = fetch_and_inc( &taskseq);  /* obtain a unique task id */
  t->func = func;
  t->inarg = inarg;

  t->stacksize = stacksize;
  if (t->stacksize <= 0) {
    t->stacksize = LPEL_TASK_ATTR_STACKSIZE_DEFAULT;
  }

  /* initialize poll token to 0 */
  atomic_init( &t->poll_token, 0);

  TASKLOCK_INIT( &t->lock);
  
  t->state = TASK_CREATED;

  t->prev = t->next = NULL;

  t->flags = 0;
  TIMESTAMP(&t->times.creat);
  t->cnt_dispatch = 0;
  t->dirty_list = STDESC_DIRTY_END; /* empty dirty list */
  
  /* function, argument (data), stack base address, stacksize */
  t->mctx = co_create( TaskStartup, (void *)t, NULL, t->stacksize);
  if (t->mctx == NULL) {
    /*TODO throw error!*/
    assert(0);
  }
  return t;
}


/**
 * Let the task run on the worker
 */
void LpelTaskRun( lpel_task_t *t)
{
  assert( t->state == TASK_CREATED );

  t->state = TASK_READY;
  _LpelWorkerRunTask( t);
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



/******************************************************************************/
/* HIDDEN FUNCTIONS                                                           */
/******************************************************************************/


/**
 * Destroy a task
 * - completely free the memory for that task
 */
void _LpelTaskDestroy( lpel_task_t *t)
{
  assert( t->state == TASK_ZOMBIE);

  atomic_destroy( &t->poll_token);

  TASKLOCK_DESTROY( &t->lock);

  /* delete the coroutine */
  co_delete(t->mctx);

  /* free the TCB itself*/
  free(t);
}





/**
 * Block a task
 */
void _LpelTaskBlock(lpel_task_t *ct, taskstate_blocked_t block_on)
{
  ct->blocked_on = block_on;
  TaskBlock( ct, TASK_BLOCKED);
}


/**
 * Unblock a task. Called from StreamRead/StreamWrite procedures
 */
void _LpelTaskUnblock( lpel_task_t *ct, lpel_task_t *blocked)
{
  assert(ct != NULL);
  assert(blocked != NULL);

  _LpelWorkerTaskWakeup( ct, blocked);
}

/**
 * Lock a task
 */
void _LpelTaskLock( lpel_task_t *t)
{
  TASKLOCK_LOCK( &t->lock );
}

/**
 * Unlock a task
 */
void _LpelTaskUnlock( lpel_task_t *t)
{
  TASKLOCK_UNLOCK( &t->lock );
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

  _LpelWorkerFinalizeTask( t->worker_context);
  TaskStart( t);

  /* call the task function with inarg as parameter */
  t->func(t, t->inarg);
  /* if task function returns, exit properly */
  LpelTaskExit(t);
}


static void TaskStart( lpel_task_t *t)
{
  /* start timestamp, dispatch counter, state */
  t->cnt_dispatch++;
  t->state = TASK_RUNNING;
      
  if ( TASK_FLAGS( t, LPEL_TASK_ATTR_MONITOR_TIMES)) {
    TIMESTAMP( &t->times.start);
  }
}

static void TaskStop( lpel_task_t *t)
{
  workerctx_t *wc = t->worker_context;
  assert( t->state != TASK_RUNNING);

  /* stop timestamp */
  if ( TASK_FLAGS( t, LPEL_TASK_ATTR_MONITOR_TIMES) ) {
    /* if monitor output, we need a timestamp */
    TIMESTAMP( &t->times.stop);
  }

  /* output accounting info */
  if ( TASK_FLAGS(t, LPEL_TASK_ATTR_MONITOR_OUTPUT)) {
    _LpelMonitoringOutput( wc->mon, t);
  }

}

static void TaskBlock( lpel_task_t *t, taskstate_t state)
{

  assert( t->state == TASK_RUNNING);
  assert( state == TASK_READY || state == TASK_ZOMBIE || state == TASK_BLOCKED);

  /* set new state */
  t->state = state;

  TaskStop( t);
  _LpelWorkerDispatcher( t);
  TaskStart( t);
}
