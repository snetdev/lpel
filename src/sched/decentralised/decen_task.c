#include <stdio.h>
#include <lpel.h>
#include "lpelcfg.h"
#include "decen_worker.h"
#include "spmdext.h"
#include "lpel/monitor.h"
#include "decen_scheduler.h"
#include "task_migration.h"

extern lpel_tm_config_t tm_conf;
static atomic_int taskseq = ATOMIC_VAR_INIT(0);

static void FinishOffCurrentTask(lpel_task_t *ct);
static void TaskStartup( void *arg);


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

	/* aligned to page boundary */
	t = valloc( size );

	/* calc stackaddr */
	offset = (sizeof(lpel_task_t) + TASK_STACK_ALIGN-1) & ~(TASK_STACK_ALIGN-1);
	stackaddr = (char *) t + offset;
	t->size = size;


	/* obtain a usable worker context */
	t->worker_context = LpelWorkerGetContext(worker);

	t->sched_info.prio = 0;

	t->uid = atomic_fetch_add( &taskseq, 1);  /* obtain a unique task id */
	t->func = func;
	t->inarg = inarg;

	/* initialize poll token to 0 */
	atomic_init( &t->poll_token, 0);

	t->state = TASK_CREATED;

	t->prev = t->next = NULL;

	t->mon = NULL;
	t->usrdata = NULL;
	t->usrdt_destr = NULL;

	/* function, argument (data), stack base address, stacksize */
	mctx_create( &t->mctx, TaskStartup, (void*)t, stackaddr, t->size - offset);
#ifdef USE_MCTX_PCL
	assert(t->mctx != NULL);
#endif

	return t;
}


/**
 * Destroy a task
 * - completely free the memory for that task
 */
void LpelTaskDestroy( lpel_task_t *t)
{
	assert( t->state == TASK_ZOMBIE);

#ifdef USE_TASK_EVENT_LOGGING
	/* if task had a monitoring object, destroy it */
	if (t->mon && MON_CB(task_destroy)) {
		MON_CB(task_destroy)(t->mon);
	}
#endif

	atomic_destroy( &t->poll_token);

	//FIXME
#ifdef USE_MCTX_PCL
	co_delete(t->mctx);
#endif

	/* free the TCB itself*/
	free(t);
}

/**
 * Set priority for a task
 * @param t				task
 * @param prio		integer priority, used for indexing the task queue
 */
void LpelTaskSetPriority(lpel_task_t *t, int prio)
{
	t->sched_info.prio = prio;
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
 * Exit the current task
 *
 * @param outarg  output argument of the task
 * @pre This call must be made within a LPEL task!
 */
void LpelTaskExit(void)
{
  lpel_task_t *ct = LpelTaskSelf();
  assert( ct->state == TASK_RUNNING );

  FinishOffCurrentTask(ct);
  /* execution never comes back here */
  assert(0);
}

/**
 * Task issues an enter world request
 */
void LpelTaskEnterSPMD( lpel_spmdfunc_t fun, void *arg)
{
	lpel_task_t *ct = LpelTaskSelf();
	workermsg_t msg;
	assert( ct->state == TASK_RUNNING );

	//FIXME conditional for availability?

	/* world request */
	LpelSpmdRequest(ct, fun, arg);

	/* broadcast message */
	msg.type = WORKER_MSG_SPMDREQ;
	msg.body.from_worker = ct->worker_context->wid;
	LpelWorkerBroadcast(&msg);

	ct->state = TASK_BLOCKED;
	/* TODO block on what? */
	TaskStop( ct);
	LpelWorkerDispatcher( ct);
	TaskStart( ct);
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

  /* task wait_prop is updated
   * check if it should be migrated
   * If yes --> self migrated, if no --> self yield
   */
  if (tm_conf.mechanism == LPEL_MIG_WAIT_PROP) {
  	int target = LpelPickTargetWorker(ct);
  	if (target > 0) {
  		TaskStop(ct);
  		LpelWorkerSelfTaskMigrate(ct, target);
  		TaskStart(ct);
  		return;
  	}
  }

  LpelWorkerSelfTaskYield(ct);
  TaskStop( ct);
  LpelWorkerDispatcher( ct);
  TaskStart( ct);
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


/**
 * Unblock a task.
 * 		Called from StreamRead/StreamWrite procedures
 * @param ct				task to unblock
 * @param blocked		task to be unblocked
 */
void LpelTaskUnblock( lpel_task_t *ct, lpel_task_t *blocked)
{
	assert(ct != NULL);
	assert(blocked != NULL);

	LpelWorkerTaskWakeup( ct, blocked);
}


/** check and migrate the current task if required, used in decen_lpel
 * to be called from snet-rts after processing one message record
 * used in random-based mechanism
 * */
void LpelTaskCheckMigrate(void) {
	if (tm_conf.mechanism != LPEL_MIG_RAND)
		return;

	lpel_task_t *t = LpelTaskSelf();
	if (t->worker_context->wid < 0)
		return;		// no migrate wrapper task

	assert( t->state == TASK_RUNNING );
	int target = LpelPickTargetWorker(t);
	if (target >= 0 && t->worker_context->wid != target) {
		  t->state = TASK_READY;
		  TaskStop(t);
		  LpelWorkerSelfTaskMigrate(t, target);
		  TaskStart(t);
	}
}


int LpelTaskGetWorkerId(lpel_task_t *t)
{
  assert(t);
  assert(t->worker_context);
  return t->worker_context->wid;
}


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
 * Get the current task
 *
 * @pre This call must be made from within a LPEL task!
 */
lpel_task_t *LpelTaskSelf(void)
{
	return LpelWorkerCurrentTask();
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

void LpelSetUserDataDestructor(lpel_task_t *t, lpel_usrdata_destructor_t destr)
{
  assert(t);
  t->usrdt_destr = destr;
}

lpel_usrdata_destructor_t LpelGetUserDataDestructor(lpel_task_t *t)
{
  assert(t);
  return t->usrdt_destr;
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

static void FinishOffCurrentTask(lpel_task_t *ct)
{
  /* call the destructor for the Task Local Data */
  if (ct->usrdt_destr && ct->usrdata) {
    ct->usrdt_destr (ct, ct->usrdata);
  }

  /* context switch happens, this task is cleaned up then */
  ct->state = TASK_ZOMBIE;
  LpelWorkerSelfTaskExit(ct);
  TaskStop( ct);
  LpelWorkerDispatcher( ct);
  /* execution never comes back here */
  assert(0);
}

