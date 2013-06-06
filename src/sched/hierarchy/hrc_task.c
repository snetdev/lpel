#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "arch/atomic.h"
#include <float.h>

#include "hrc_task.h"
#include "hrc_stream.h"
#include "lpelcfg.h"
#include "hrc_worker.h"
#include "lpel/monitor.h"
#include "taskpriority.h"

static atomic_int taskseq = ATOMIC_VAR_INIT(0);

static double (*prior_cal) (int in, int out) = priorfunc14;

static void TaskStartup( void *arg);

static void TaskStart( lpel_task_t *t);
static void TaskStop( lpel_task_t *t);


#define TASK_STACK_ALIGN  256
#define TASK_MINSIZE  4096


/**
 * Create a task.
 *
 * @param worker  id of the worker where to create the task (0 = master, -1 = others, -2 = sosi)
 * @param func    task function
 * @param arg     arguments
 * @param size    size of the task, including execution stack
 * @param sc			scheduling condition, decide when a task should yield
 * @pre           size is a power of two, >= 4096
 *
 * @return the task handle of the created task (pointer to TCB)
 *
 */
lpel_task_t *LpelTaskCreate( int map, lpel_taskfunc_t func,
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


	if (map != LPEL_MAP_MASTER )	/** others wrapper or source/sink */
		t->worker_context = LpelCreateWrapperContext(map);
	else
		t->worker_context = NULL;

	t->uid = atomic_fetch_add( &taskseq, 1);  /* obtain a unique task id */
	t->func = func;
	t->inarg = inarg;

	/* initialize poll token to 0 */
	atomic_init( &t->poll_token, 0);

	t->state = TASK_CREATED;
	t->wakedup = 0;

	t->prev = t->next = NULL;

	t->mon = NULL;

	/* function, argument (data), stack base address, stacksize */
	mctx_create( &t->mctx, TaskStartup, (void*)t, stackaddr, t->size - offset);
#ifdef USE_MCTX_PCL
	assert(t->mctx != NULL);
#endif

	// default scheduling info
	t->sched_info.prior = 0;
	t->sched_info.rec_cnt = 0;
	t->sched_info.rec_limit = 0;
	t->sched_info.rec_limit_factor = -1;
	t->sched_info.in_streams = NULL;
	t->sched_info.out_streams = NULL;

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


	assert(t->sched_info.in_streams == NULL);
	assert(t->sched_info.out_streams == NULL);
	/* free the TCB itself*/
	free(t);
}


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
void LpelTaskExit(void)
{
	lpel_task_t *ct = LpelTaskSelf();
	assert( ct->state == TASK_RUNNING );

	/* context switch happens, this task is cleaned up then */
	ct->state = TASK_ZOMBIE;
	TaskStop( ct);
	LpelWorkerTaskExit(ct);
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
	TaskStop( ct);
	LpelWorkerTaskYield(ct);
	TaskStart( ct);
}




/**
 * Block a task
 */
void LpelTaskBlockStream(lpel_task_t *t)
{
	/* a reference to it is held in the stream */
	assert( t->state == TASK_RUNNING );
	t->state = TASK_BLOCKED;
	TaskStop( t);
	LpelWorkerTaskBlock(t);
	TaskStart( t);		// task will be backed here when it is dispatched the next time

}


/**
 * Unblock a task. Called from StreamRead/StreamWrite procedures
 */
void LpelTaskUnblock( lpel_task_t *t)
{
	assert(t != NULL);
	LpelWorkerTaskWakeup( t);
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
	TaskStop(t);
	LpelWorkerTaskExit(t);
	/* execution never comes back here */
	assert(0);
}


static void TaskStart( lpel_task_t *t)
{
	// TODO reset task scheduling info

	assert( t->state == TASK_READY );
	/* MONITORING CALLBACK */
#ifdef USE_TASK_EVENT_LOGGING
	if (t->mon && MON_CB(task_start)) {
		MON_CB(task_start)(t->mon);
	}
#endif

	t->sched_info.rec_cnt = 0;	// reset rec_cnt
	t->state = TASK_RUNNING;
}


static void TaskStop( lpel_task_t *t)
{
	/* MONITORING CALLBACK */
#ifdef USE_TASK_EVENT_LOGGING
	if (t->mon && MON_CB(task_stop)) {
		MON_CB(task_stop)(t->mon, t->state);
	}
#endif

}


/*
 * this will be called before 1 rec is processed
 * increase the rec count by 1, if it reaches the limit then yield
 */
void LpelTaskCheckYield(lpel_task_t *t) {

	assert( t->state == TASK_RUNNING );

	if (t->sched_info.rec_limit < 0) {		//limit < 0 --> no yield
		return;
	}

	if (t->sched_info.rec_cnt == t->sched_info.rec_limit) {
		t->state = TASK_READY;
		TaskStop( t);
		LpelWorkerTaskYield(t);
		TaskStart( t);
	}
	t->sched_info.rec_cnt ++;

}

void LpelTaskSetRecLimit(lpel_task_t *t, int lim) {
	t->sched_info.rec_limit_factor = lim;
}

void LpelTaskSetPrior(lpel_task_t *t, double p) {
	t->sched_info.prior = p;
}


void LpelTaskAddStream( lpel_task_t *t, lpel_stream_desc_t *des, char mode) {
	stream_elem_t **list;
	stream_elem_t *head;
	switch (mode) {
	case 'r':
		list = &t->sched_info.in_streams;
		break;
	case 'w':
		list = &t->sched_info.out_streams;
		t->sched_info.rec_limit += t->sched_info.rec_limit_factor;
		break;
	}
	head = *list;
	stream_elem_t *new = (stream_elem_t *) malloc(sizeof(stream_elem_t));
	new->stream_desc = des;
	if (head)
		new->next = head;
  else
		new->next = NULL;
	*list = new;
}


void LpelTaskRemoveStream( lpel_task_t *t, lpel_stream_desc_t *des, char mode) {
	stream_elem_t **list;
	stream_elem_t *head;
	switch (mode) {
	case 'r':
		list = &t->sched_info.in_streams;
		break;
	case 'w':
		list = &t->sched_info.out_streams;
		t->sched_info.rec_limit -= t->sched_info.rec_limit_factor;
		break;
	}
	head = *list;

	stream_elem_t *prev = NULL;

	while (head != NULL) {
		if (head->stream_desc == des)
			break;
		prev = head;
		head = head->next;
	}

	assert(head != NULL);		//item must be in the list
	if (prev == NULL)
		*list = head->next;
	else
		prev->next = head->next;

	free(head);
}



int countRec(stream_elem_t *list, char inout) {
	if (list == NULL)
		return -1;
	int cnt = 0;
	int flag = 0;
	while (list != NULL) {
		if (list->stream_desc->stream) {
			if ((inout == 'i' && list->stream_desc->stream->type == LPEL_STREAM_ENTRY)
					|| (inout == 'o' && list->stream_desc->stream->type == LPEL_STREAM_EXIT)) {
				// if input stream is entry or output stream is exit --> not count
				flag = 1;
			} else
				cnt += LpelStreamFillLevel(list->stream_desc->stream);
		}
		list = list->next;
	}
	if (flag == 1 && cnt == 0)
		cnt = -1;
	return cnt;
}


double LpelTaskCalPriority(lpel_task_t *t) {
	int in, out;
	in = countRec(t->sched_info.in_streams, 'i');
	out = countRec(t->sched_info.out_streams, 'o');

#ifdef _USE_NEG_DEMAND_LIMIT_
	/* if t is entry task and already produced too many ouput, set it to DBL_MIN and it will not be scheduled */
	if (in == -1 && out > NEG_DEMAND_LIMIT)
		return DBL_MIN;
#endif

	return prior_cal(in, out);
}

void LpelTaskSetPriorityFunc(int func){
	switch (func){
	case 1: prior_cal = priorfunc1;
					break;
	case 2: prior_cal = priorfunc2;
					break;
	case 3: prior_cal = priorfunc3;
					break;
	case 4: prior_cal = priorfunc4;
					break;
	case 5: prior_cal = priorfunc5;
					break;
	case 6: prior_cal = priorfunc6;
					break;
	case 7: prior_cal = priorfunc7;
					break;
	case 8: prior_cal = priorfunc8;
					break;
	case 9: prior_cal = priorfunc9;
					break;
	case 10: prior_cal = priorfunc10;
					break;
	case 11: prior_cal = priorfunc11;
					break;
	case 12: prior_cal = priorfunc12;
					break;
	case 13: prior_cal = priorfunc13;
					break;
	case 14: prior_cal = priorfunc14;
					break;
	default: prior_cal = priorfunc14;
	}
}

int LpelTaskGetWorkerId(lpel_task_t *t) {
	if (t->worker_context)
		return t->worker_context->wid;
	else
		return -1;
}


void LpelTaskCheckMigrate(void) {}

int LpelTaskIsWrapper(lpel_task_t *t) {
	return (t->worker_context->wid == -1);
}
