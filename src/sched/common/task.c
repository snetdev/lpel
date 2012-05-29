/**
 * File: stream.c
 * Auth: Daniel Prokesch <daniel.prokesch@gmail.com>
 * Modified: Nga
 *
 * Desc: Common implementations of task for DECEN and HRC
 *
 */

#include <stdlib.h>
#include <assert.h>

#include "arch/atomic.h"

#include "task.h"

#include "lpelcfg.h"
#include "worker.h"
#include "stream.h"
#include "lpel/monitor.h"

static void FinishOffCurrentTask(lpel_task_t *ct);

/**
 * Get Task Id
 *		usually used for debugging
 * @param t		task
 */
unsigned int LpelTaskGetId(lpel_task_t *t)
{
  return t->uid;
}

/**
 * Get Task Monitor
 *
 * @param t		task
 */
mon_task_t *LpelTaskGetMon( lpel_task_t *t )
{
  return t->mon;
}

/**
 * Set Task Monitor
 *
 * @param t		task
 * @param mt	task monitor
 */
void LpelTaskMonitor(lpel_task_t *t, mon_task_t *mt)
{
  t->mon = mt;
}


/**
 * Let a task start
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
  lpel_task_t *t = LpelWorkerCurrentTask();
  /* It is quite a common bug to call LpelTaskSelf() from a non-task context.
   * Provide an assertion error instead of just segfaulting on a null dereference. */
  assert(t && "Not in an LPEL task context!");
  return t;
}


/** user data */
void  LpelSetUserData(lpel_task_t *t, void *data)
{
  assert(t);
  t->usrdata = data;
}

void *LpelGetUserData(lpel_task_t *t)
{
  assert(t);
  return t->usrdata;
}


/**
 * Exit the current task
 *
 * @param outarg  output argument of the task
 * @pre This call must be made within a LPEL task!
 */
void LpelTaskExit(void *outarg)
{
  lpel_task_t *ct = LpelTaskSelf();
  assert( ct->state == TASK_RUNNING );

  ct->outarg = outarg;

  FinishOffCurrentTask(ct);
  /* execution never comes back here */
  assert(0);
}




/**
 * Block a task by reading from/writing to a stream
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

static void FinishOffCurrentTask(lpel_task_t *ct)
{
  /* call the destructor for the Task Local Data */
  if (ct->usrdt_destr && ct->usrdata) {
    ct->usrdt_destr (ct, ct->usrdata);
  }

  /* context switch happens, this task is cleaned up then */
  ct->state = TASK_ZOMBIE;
  LpelWorkerSelfTaskExit(ct);
  LpelTaskBlock( ct );
  /* execution never comes back here */
  assert(0);
}
