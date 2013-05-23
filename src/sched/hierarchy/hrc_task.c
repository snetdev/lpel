#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "arch/atomic.h"

#include "hrc_task.h"
#include "hrc_stream.h"
#include "lpelcfg.h"
#include "hrc_worker.h"
#include "lpel/monitor.h"
#include "taskpriority.h"

static atomic_int taskseq = ATOMIC_VAR_INIT(0);

static double (*prior_cal) (int in, int out) = priorfunc14;


int countRec(stream_elem_t *list, char inout);

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


	if (map == LPEL_MAP_OTHERS )	/** others wrapper or source/sink */
		t->worker_context = LpelCreateWrapperContext(map);
	else
		t->worker_context = NULL;

	t->uid = atomic_fetch_add( &taskseq, 1);  /* obtain a unique task id */
	t->func = func;
	t->inarg = inarg;

	/* initialize poll token to 0 */
	atomic_init( &t->poll_token, 0);

	t->state = TASK_CREATED;

	t->prev = t->next = NULL;

	t->mon = NULL;

	/* function, argument (data), stack base address, stacksize */
	mctx_create( &t->mctx, TaskStartup, (void*)t, stackaddr, t->size - offset);
#ifdef USE_MCTX_PCL
	assert(t->mctx != NULL);
#endif

	// default scheduling info
	t->sched_info = (sched_task_t *) malloc(sizeof(sched_task_t));
	t->sched_info->prior = 0;
	t->sched_info->rec_cnt = 0;
	t->sched_info->rec_limit = LPEL_REC_LIMIT_DEFAULT;
	t->sched_info->in_streams = NULL;
	t->sched_info->out_streams = NULL;

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


	assert(t->sched_info->in_streams == NULL);
	assert(t->sched_info->out_streams == NULL);
	free(t->sched_info);
	/* free the TCB itself*/
	free(t);
}



/**
 * Unblock a task. Called from StreamRead/StreamWrite procedures
 */
void LpelTaskUnblock( lpel_task_t *by, lpel_task_t *t)
{
	assert(t != NULL);
	LpelWorkerTaskWakeup( t);
}




/*
 * this will be called after producing each record
 * increase the rec count by 1, if it reaches the limit then yield
 */
void LpelTaskCheckYield(lpel_task_t *t) {

	assert( t->state == TASK_RUNNING );

	if (t->sched_info->rec_limit < 0) {		//limit < 0 --> no yield
		return;
	}

	t->sched_info->rec_cnt ++;
	if (t->sched_info->rec_cnt >= t->sched_info->rec_limit) {
		t->state = TASK_READY;
		TaskStop( t);
		LpelWorkerSelfTaskYield(t);
		TaskStart( t);
	}
}

/*
 * set limit of produced records
 */
void LpelTaskSetRecLimit(lpel_task_t *t, int lim) {
	t->sched_info->rec_limit = lim;
}

void LpelTaskSetPriority(lpel_task_t *t, double p) {
	t->sched_info->prior = p;
}

/*
 * Add a tracked stream, used for calculate task priority
 * @param t			task
 * @param des		stream description
 * @param mode	read/write mode
 */
void LpelTaskAddStream( lpel_task_t *t, lpel_stream_desc_t *des, char mode) {
	stream_elem_t **list;
	stream_elem_t *head;
	switch (mode) {
	case 'r':
		list = &t->sched_info->in_streams;
		break;
	case 'w':
		list = &t->sched_info->out_streams;
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

/*
 * Remove a tracked stream
 * @param t			task
 * @param des		stream description
 * @param mode	read/write mode
 */
void LpelTaskRemoveStream( lpel_task_t *t, lpel_stream_desc_t *des, char mode) {
	stream_elem_t **list;
	stream_elem_t *head;
	switch (mode) {
	case 'r':
		list = &t->sched_info->in_streams;
		break;
	case 'w':
		list = &t->sched_info->out_streams;
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



/*
 * Calculate dynamic priority for task
 */
double LpelTaskCalPriority(lpel_task_t *t) {
	int in, out;
	in = countRec(t->sched_info->in_streams, 'i');
	out = countRec(t->sched_info->out_streams, 'o');
	return prior_cal(in, out);
}


/*
 * Set function used to calculate task priority
 */
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
	case 13: prior_cal = priorfunc13;	// random
					break;
	case 14: prior_cal = priorfunc14;
					break;

	default: prior_cal = priorfunc1;
	}
}

/*
 * get WorkerId where task is currently running
 * 		used for debugging
 */
int LpelTaskGetWorkerId(lpel_task_t *t) {
	if (t->worker_context)
		return t->worker_context->wid;
	else
		return -1;
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
 * Check if a task is wrapper, used for set entry/exit stream
 */
int LpelTaskIsWrapper(lpel_task_t *t) {
	assert(t != NULL);
	return LpelWorkerIsWrapper(t->worker_context);
}


/*
 * void function to provide task migration in lpel decen
 * */
void LpelTaskCheckMigrate(void) {
}

/******************************************************************************/
/* PRIVATE FUNCTIONS                                                          */
/******************************************************************************/
void TaskStart( lpel_task_t *t)
{
	assert( t->state == TASK_READY );

	/* MONITORING CALLBACK */
#ifdef USE_TASK_EVENT_LOGGING
	if (t->mon && MON_CB(task_start)) {
		MON_CB(task_start)(t->mon);
	}
#endif
	t->sched_info->rec_cnt = 0;	// reset rec_cnt
	t->state = TASK_RUNNING;
}

/*
 * count records in the list of tracked stream
 */
int countRec(stream_elem_t *list, char inout) {
	if (list == NULL)
		return -1;
	int cnt = 0;
	while (list != NULL) {
		if (list->stream_desc->stream) {
			if ((inout == 'i' && LpelStreamIsEntry(list->stream_desc->stream))
					|| (inout == 'o' && LpelStreamIsExit(list->stream_desc->stream))) {
				// if input stream is entry or output stream is exit --> not count
			} else
				cnt += LpelStreamFillLevel(list->stream_desc->stream);
		}
		list = list->next;
	}
	return cnt;
}
