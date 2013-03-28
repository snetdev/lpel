
#include <stdlib.h>
#include <assert.h>

#include "arch/atomic.h"

#include "task.h"

#include "lpelcfg.h"
#include "worker.h"
#include "stream.h"
#include "lpel/monitor.h"


unsigned int LpelTaskGetId(lpel_task_t *t)
{
  return t->uid;
}

mon_task_t *LpelTaskGetMon( lpel_task_t *t )
{
  return t->mon;
}


void LpelTaskMonitor(lpel_task_t *t, mon_task_t *mt)
{
  t->mon = mt;
}


/**
 * Let the task run on the worker
 */
void LpelTaskStart( lpel_task_t *t)
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
  ct->state = TASK_ZOMBIE;
  LpelWorkerSelfTaskExit(ct);
  TaskStop( ct);
  LpelWorkerDispatcher( ct);
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

  ct->state = TASK_READY;
  LpelWorkerSelfTaskYield(ct);
  TaskStop( ct);
  LpelWorkerDispatcher( ct);
  TaskStart( ct);
}




/**
 * Block a task
 */
void LpelTaskBlockStream(lpel_task_t *t)
{
  /* a reference to it is held in the stream */
  t->state = TASK_BLOCKED;
  LpelWorkerTaskBlock(t);
  TaskStop( t);
  LpelWorkerDispatcher( t);
  TaskStart( t);
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
void TaskStartup( void *data)
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
  LpelWorkerSelfTaskExit(t);
  TaskStop( t);
  LpelWorkerDispatcher( t);
  /* execution never comes back here */
  assert(0);
}


void TaskStart( lpel_task_t *t)
{
  assert( t->state == TASK_READY );

  /* MONITORING CALLBACK */
#ifdef USE_TASK_EVENT_LOGGING
  if (t->mon && MON_CB(task_start)) {
    MON_CB(task_start)(t->mon);
  }
#endif

  t->state = TASK_RUNNING;
}


void TaskStop( lpel_task_t *t)
{
  //workerctx_t *wc = t->worker_context;
  assert( t->state != TASK_RUNNING);

  /* MONITORING CALLBACK */
#ifdef USE_TASK_EVENT_LOGGING
  if (t->mon && MON_CB(task_stop)) {
    MON_CB(task_stop)(t->mon, t->state);
  }
#endif

}




