#include <stdio.h>
#include "task.h"
#include "lpelcfg.h"
#include "decen_worker.h"
#include "spmdext.h"
#include "stream.h"
#include "lpel/monitor.h"
#include "decen_scheduler.h"
#include "lpel.h"
#include "task_migration.h"

extern lpel_tm_config_t tm_conf;
static atomic_int taskseq = ATOMIC_VAR_INIT(0);

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

	t->sched_info = (sched_task_t *) malloc(sizeof(sched_task_t));
	t->sched_info->prio = 0;

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
	free(t->sched_info);
	free(t);
}

/**
 * Set priority for a task
 * @param t				task
 * @param prio		integer priority, used for indexing the task queue
 */
void LpelTaskSetPriority(lpel_task_t *t, int prio)
{
	t->sched_info->prio = prio;
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


/* private function */
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


int LpelTaskGetWorkerId(lpel_task_t *t)
{
  assert(t);
  assert(t->worker_context);
  return t->worker_context->wid;
}
