
#include <stdlib.h>
#include <assert.h>

#include "arch/atomic.h"

#include "task.h"

#include "worker.h"
#include "workerctx.h"
#include "stream.h"

#include "lpel_main.h"



static atomic_t taskseq = ATOMIC_INIT(0);



/* declaration of startup function */
//static void TaskStartup( unsigned int y, unsigned int x);
static void TaskStartup( void *arg);

static void TaskStart( lpel_task_t *t);
static void TaskStop( lpel_task_t *t);

typedef enum lpel_taskstate_t lpel_taskstate_t;

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
lpel_task_t *LPEL_EXPORT(TaskCreate)( int worker, lpel_taskfunc_t func,
    void *inarg, int size)
{
  lpel_task_t *t;
  char *stackaddr;
  int offset;

  if (size <= 0) {
    size = LPEL_TASK_SIZE_DEFAULT;
  }
  assert( size >= TASK_MINSIZE );

  /* aligned to page boundary */
  t = valloc( size );

  /* calc stackaddr */
  offset = (sizeof(lpel_task_t) + TASK_STACK_ALIGN-1) & ~(TASK_STACK_ALIGN-1);
  stackaddr = (char *) t + offset;
  t->size = size;


  /* obtain a usable worker context */
  t->worker_context = LPEL_EXPORT(WorkerGetContext)(worker);

  t->sched_info.prio = 0;

  t->uid = fetch_and_inc( &taskseq);  /* obtain a unique task id */
  t->func = func;
  t->inarg = inarg;

  /* initialize poll token to 0 */
  atomic_init( &t->poll_token, 0);

  t->state = TASK_CREATED;

  t->prev = t->next = NULL;

  t->mon = NULL;

  /* function, argument (data), stack base address, stacksize */
  mctx_create( &t->mctx, TaskStartup, (void*)t, stackaddr, t->size - offset);
  assert(t->mctx != NULL);

  return t;
}



/**
 * Destroy a task
 * - completely free the memory for that task
 */
void LPEL_EXPORT(TaskDestroy)( lpel_task_t *t)
{
  assert( t->state == TASK_ZOMBIE);

  /* if task had a monitoring object, destroy it */
  if (t->mon && MON_CB(task_destroy)) {
    MON_CB(task_destroy)(t->mon);
  }

  atomic_destroy( &t->poll_token);

//FIXME
#ifdef USE_MCTX_PCL
  co_delete(t->mctx);
#endif

  /* free the TCB itself*/
  free(t);
}


unsigned int LPEL_EXPORT(TaskGetID)(lpel_task_t *t)
{
  return t->uid;
}

mon_task_t *LPEL_EXPORT(TaskGetMon)( lpel_task_t *t )
{
  return t->mon;
}


void LPEL_EXPORT(TaskMonitor)(lpel_task_t *t, mon_task_t *mt)
{
  t->mon = mt;
}


void LPEL_EXPORT(TaskPrio)(lpel_task_t *t, int prio)
{
  t->sched_info.prio = prio;
}


/**
 * Let the task run on the worker
 */
void LPEL_EXPORT(TaskRun)( lpel_task_t *t)
{
  assert( t->state == TASK_CREATED );

  LPEL_EXPORT(WorkerRunTask)( t);
}



/**
 * Get the current task
 *
 * @pre This call must be made from within a LPEL task!
 */
lpel_task_t *LPEL_EXPORT(TaskSelf)(void)
{
  return LPEL_EXPORT(WorkerCurrentTask)();
}


/**
 * Exit the current task
 *
 * @param outarg  output argument of the task
 * @pre This call must be made from within a LPEL task!
 */
void LPEL_EXPORT(TaskExit)(void *outarg)
{
  lpel_task_t *ct = LPEL_EXPORT(TaskSelf)();
  assert( ct->state == TASK_RUNNING );

  ct->outarg = outarg;

  /* context switch happens, this task is cleaned up then */
  ct->state = TASK_ZOMBIE;
  LPEL_EXPORT(WorkerSelfTaskExit)(ct);
  LPEL_EXPORT(TaskBlock)( ct );
  /* execution never comes back here */
  assert(0);
}


/**
 * Yield execution back to scheduler voluntarily
 *
 * @pre This call must be made from within a LPEL task!
 */
void LPEL_EXPORT(TaskYield)(void)
{
  lpel_task_t *ct = LPEL_EXPORT(TaskSelf)();
  assert( ct->state == TASK_RUNNING );

  ct->state = TASK_READY;
  LPEL_EXPORT(WorkerSelfTaskYield)(ct);
  LPEL_EXPORT(TaskBlock)( ct );
}




/**
 * Block a task
 */
void LPEL_EXPORT(TaskBlockStream)(lpel_task_t *t)
{
  /* a reference to it is held in the stream */
  t->state = TASK_BLOCKED;
  LPEL_EXPORT(TaskBlock)( t );
}


/**
 * Unblock a task. Called from StreamRead/StreamWrite procedures
 */
void LPEL_EXPORT(TaskUnblock)( lpel_task_t *ct, lpel_task_t *blocked)
{
  assert(ct != NULL);
  assert(blocked != NULL);

  LPEL_EXPORT(WorkerTaskWakeup)( ct, blocked);
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
  t->state = TASK_ZOMBIE;
  LPEL_EXPORT(WorkerSelfTaskExit)(t);
  LPEL_EXPORT(TaskBlock)( t );
  /* execution never comes back here */
  assert(0);
}


static void TaskStart( lpel_task_t *t)
{
  assert( t->state == TASK_READY );

  /* MONITORING CALLBACK */
  if (t->mon && MON_CB(task_start)) {
    MON_CB(task_start)(t->mon);
  }

  t->state = TASK_RUNNING;
}

static void TaskStop( lpel_task_t *t)
{
  //workerctx_t *wc = t->worker_context;
  assert( t->state != TASK_RUNNING);

  /* MONITORING CALLBACK */
  if (t->mon && MON_CB(task_stop)) {
    MON_CB(task_stop)(t->mon, t->state);
  }
}


void LPEL_EXPORT(TaskBlock)( lpel_task_t *t )
{
  assert( t->state != TASK_RUNNING);

  TaskStop( t);
  LPEL_EXPORT(WorkerDispatcher)( t);
  TaskStart( t);
}


