/*
 * hrc_worker_op.c
 *
 *  Created on: 17 Jul 2013
 *      Author: administrator
 */

/*
 * The LPEL worker containing code for workers, master and wrappers
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <errno.h>
#include <float.h>

#include <pthread.h>
#include "arch/mctx.h"

#include "arch/atomic.h"

#include "hrc_worker.h"
#include "hrc_task.h"
#include "lpel_hwloc.h"
#include "lpelcfg.h"
#include "hrc_stream.h"
#include "mailbox.h"
#include "lpel/monitor.h"
#include "lpel_main.h"

#define MASTER_PTR	master

//#define _USE_WORKER_DBG__

#ifdef _USE_WORKER_DBG__
#define WORKER_DBG printf
#else
#define WORKER_DBG	//
#endif



/******************* PRIVATE FUNCTIONS *****************************/
static void addFreeWrapper(workerctx_t *wp);
static workerctx_t *getFreeWrapper();

/******************************************************************************/
static int num_workers = -1;
static mailbox_t *mastermb;
static mailbox_t **workermbs;

static workerctx_t *freewrappers;
static PRODLOCK_TYPE lockwrappers;

#ifdef HAVE___THREAD
static TLSSPEC workerctx_t *workerctx_cur;
#else /* HAVE___THREAD */
static pthread_key_t workerctx_key;
#endif /* HAVE___THREAD */
/******************************************************************************/

void initLocalVar(int size){
#ifndef HAVE___THREAD
	/* init key for thread specific data */
	pthread_key_create(&workerctx_key, NULL);
#endif /* HAVE___THREAD */

  /* free wrappers */
  freewrappers = NULL;
	PRODLOCK_INIT(&lockwrappers);
  num_workers = size;
  /* mailboxes */
  workermbs = (mailbox_t *) malloc(sizeof(mailbox_t *) * num_workers);
  setupMailbox(&mastermb, workermbs);
}

void cleanupLocalVar(){
#ifndef HAVE___THREAD
	pthread_key_delete(workerctx_key);
#endif /* HAVE___THREAD */

  /* free wrappers */
	workerctx_t *wp = freewrappers;
	workerctx_t *next;
	while (wp != NULL) {
		next = wp->next;
		LpelMailboxDestroy(wp->mailbox);
		LpelWorkerDestroyStream(wp);
		LpelWorkerDestroySd(wp);
		free(wp);
		wp = next;
	}
	PRODLOCK_DESTROY(&lockwrappers);
  
  /* mailboxes */
  free(workermbs);
}


/**
 * Assign a task to the worker by sending an assign message to that worker
 */
void LpelWorkerRunTask(lpel_task_t *t) {
	 workermsg_t msg;
   msg.type = WORKER_MSG_ASSIGN;
	 msg.body.task = t;

	if (t->worker_context != NULL) {	// wrapper
		LpelMailboxSend(t->worker_context->mailbox, &msg);
	}	else {
		LpelMailboxSend(mastermb, &msg);
	}
}


static void returnTask(lpel_task_t *t) {
	workermsg_t msg;
	msg.type = WORKER_MSG_RETURN;
	msg.body.task = t;
	LpelMailboxSend(mastermb, &msg);
}


static void requestTask(workerctx_t *wc) {
	WORKER_DBG("worker %d: request task\n", wc->wid);
	workermsg_t msg;
	msg.type = WORKER_MSG_REQUEST;
	msg.body.from_worker = wc->wid;
	LpelMailboxSend(mastermb, &msg);
#ifdef USE_LOGGING
	if (wc->mon && MON_CB(worker_waitstart)) {
		MON_CB(worker_waitstart)(wc->mon);
	}
#endif
}


static void sendTask(int wid, lpel_task_t *t) {
	assert(t->state == TASK_READY);
	workermsg_t msg;
	msg.type = WORKER_MSG_ASSIGN;
	msg.body.task = t;
	LpelMailboxSend(workermbs[wid], &msg);
 }

static void sendWakeup(mailbox_t *mb, lpel_task_t *t)
{
	workermsg_t msg;
	msg.type = WORKER_MSG_WAKEUP;
	msg.body.task = t;
	LpelMailboxSend(mb, &msg);
}

/*******************************************************************************
 * MASTER FUNCTION
 ******************************************************************************/
static int servePendingReq(lpel_task_t *t) {
	int i;
	t->sched_info.prior = LpelTaskCalPriority(t);
	for (i = 0; i < num_workers; i++){
		if (master->waitworkers[i] == 1) {
			master->waitworkers[i] = 0;
			WORKER_DBG("master: send task %d to worker %d\n", t->uid, i);
			sendTask(i, t);
			return i;
		}
	}
	return -1;
}

static void updatePriorityList(taskqueue_t *tq, stream_elem_t *list, char mode) {
	double np;
	lpel_task_t *t;
	lpel_stream_t *s;
	while (list != NULL) {
		s = list->stream_desc->stream;
		if (mode == 'r')
			t = LpelStreamProducer(s);
		else if (mode == 'w')
			t = LpelStreamConsumer(s);
		if (t && t->state == TASK_INQUEUE) {
			np = LpelTaskCalPriority(t);
			LpelTaskqueueUpdatePriority(tq, t, np);
		}
		list = list->next;
	}
}

/* update prior for neighbors of t
 * @cond: called only by master to avoid concurrent access
 */
static void updatePriorityNeigh(taskqueue_t *tq, lpel_task_t *t) {
	updatePriorityList(tq, t->sched_info.in_streams, 'r');
	updatePriorityList(tq, t->sched_info.out_streams, 'w');
}


static void MasterLoop(void)
{
	WORKER_DBG("start master\n");
	do {
		workermsg_t msg;

		LpelMailboxRecv(mastermb, &msg);
		lpel_task_t *t;
		int wid;
		switch(msg.type) {
		case WORKER_MSG_ASSIGN:
			/* master receive a new task */
			t = msg.body.task;
			assert (t->state == TASK_CREATED);
			t->state = TASK_READY;
			WORKER_DBG("master: get task %d\n", t->uid);
			if (servePendingReq(t) < 0) {		 // no pending request
				t->sched_info.prior = DBL_MAX; //created task does not set up input/output stream yet, set as highest priority
				t->state = TASK_INQUEUE;
				LpelTaskqueuePush(MASTER_PTR->ready_tasks, t);
			}
			break;

		case WORKER_MSG_RETURN:
			t = msg.body.task;
			WORKER_DBG("master: get returned task %d\n", t->uid);
			switch(t->state) {
			case TASK_BLOCKED:
				if (t->wakedup == 1) {	/* task has been waked up */
					t->wakedup = 0;
					t->state = TASK_READY;
					// no break, task will be treated as if it is returned as ready
				} else {
					t->state = TASK_RETURNED;
					updatePriorityNeigh(MASTER_PTR->ready_tasks, t);
					break;
				}

			case TASK_READY:	// task yields
#ifdef _USE_NEG_DEMAND_LIMIT_
				t->sched_info.prior = LpelTaskCalPriority(t);
				if (t->sched_info.prior == LPEL_DBL_MIN) {		// if not schedule task if it has too low priority
					t->state = TASK_INQUEUE;
					LpelTaskqueuePush(MASTER_PTR->ready_tasks, t);
					break;
				}
#endif
				if (servePendingReq(t) < 0) {		// no pending request
					updatePriorityNeigh(MASTER_PTR->ready_tasks, t);
					t->sched_info.prior = LpelTaskCalPriority(t);	//update new prior before add to the queue
					t->state = TASK_INQUEUE;
					LpelTaskqueuePush(MASTER_PTR->ready_tasks, t);
				}
				break;

			case TASK_ZOMBIE:
				updatePriorityNeigh(MASTER_PTR->ready_tasks, t);
				LpelTaskDestroy(t);
				break;
			default:
				assert(0);
				break;
			}
			break;

		case WORKER_MSG_WAKEUP:
			t = msg.body.task;
			if (t->state != TASK_RETURNED) {		// task has not been returned yet
				t->wakedup = 1;		// set task as wakedup so that when returned it will be treated as ready
				break;
			}
			WORKER_DBG("master: unblock task %d\n", t->uid);
			t->state = TASK_READY;

#ifdef _USE_NEG_DEMAND_LIMIT_
				t->sched_info.prior = LpelTaskCalPriority(t);
				if (t->sched_info.prior == LPEL_DBL_MIN) {		// if not schedule task if it has too low priority
					t->state = TASK_INQUEUE;
					LpelTaskqueuePush(MASTER_PTR->ready_tasks, t);
					break;
				}
#endif

			if (servePendingReq(t) < 0) {		// no pending request
#ifndef _USE_NEG_DEMAND_LIMIT_
					t->sched_info.prior = LpelTaskCalPriority(t);	//update new prior before add to the queue
#endif
					t->state = TASK_INQUEUE;
					LpelTaskqueuePush(MASTER_PTR->ready_tasks, t);
			}
			break;


		case WORKER_MSG_REQUEST:
			wid = msg.body.from_worker;
			WORKER_DBG("master: request task from worker %d\n", wid);
			t = LpelTaskqueuePeek(MASTER_PTR->ready_tasks);
			if (t == NULL) {
				master->waitworkers[wid] = 1;
			} else {

#ifdef _USE_NEG_DEMAND_LIMIT_
				if (t->sched_info.prior == LPEL_DBL_MIN) {		// if not schedule task if it has too low priority
					master->waitworkers[wid] = 1;
					break;
				}
#endif
				t->state = TASK_READY;
				sendTask(wid, t);
				t = LpelTaskqueuePop(MASTER_PTR->ready_tasks);
			}
			break;

		case WORKER_MSG_TERMINATE:
			MASTER_PTR->terminate = 1;
			break;
		default:
			assert(0);
		}
	} while (!(MASTER_PTR->terminate && LpelTaskqueueSize(MASTER_PTR->ready_tasks) == 0));
}



void *MasterThread(void *arg)
{
  master = (masterctx_t *)arg;
  num_workers = master->num_workers;
//#ifdef HAVE___THREAD
//  workerctx_cur = ms;
//#else /* HAVE___THREAD */
//  /* set pointer to worker context as TSD */
//  pthread_setspecific(workerctx_key, ms);
//#endif /* HAVE___THREAD */


//FIXME
#ifdef USE_MCTX_PCL
  assert(0 == co_thread_init());
  master->mctx = co_current();
#endif


  /* assign to cores */
  master->terminate = 0;
  LpelThreadAssign(LPEL_MAP_MASTER);

  // master loop, no monitor for master
  MasterLoop();

  // master terminated, now terminate worker
  workermsg_t msg;
  msg.type = WORKER_MSG_TERMINATE;
  LpelWorkerBroadcast(&msg);

#ifdef USE_MCTX_PCL
  co_thread_cleanup();
#endif

  return NULL;
}


/*******************************************************************************
 * WRAPPER FUNCTION
 ******************************************************************************/


static void WrapperLoop(workerctx_t *wp)
{
	lpel_task_t *t = NULL;
	workermsg_t msg;

	do {
		t = wp->current_task;
		if (t != NULL) {
			/* execute task */
			mctx_switch(&wp->mctx, &t->mctx);
		} else {
			/* no ready tasks */
			LpelMailboxRecv(wp->mailbox, &msg);
			switch(msg.type) {
			case WORKER_MSG_ASSIGN:
				t = msg.body.task;
				WORKER_DBG("wrapper: get task %d\n", t->uid);
				assert(t->state == TASK_CREATED);
				t->state = TASK_READY;
				wp->current_task = t;
#ifdef USE_LOGGING
				if (t->mon) {
					if (MON_CB(worker_create_wrapper)) {
						wp->mon = MON_CB(worker_create_wrapper)(t->mon);
					} else {
						wp->mon = NULL;
					}
				}
				if (t->mon && MON_CB(task_assign)) {
					MON_CB(task_assign)(t->mon, wp->mon);
				}
#endif
				break;

			case WORKER_MSG_WAKEUP:
				t = msg.body.task;
				WORKER_DBG("wrapper: unblock task %d\n", t->uid);
				assert (t->state == TASK_BLOCKED);
				t->state = TASK_READY;
				wp->current_task = t;
#ifdef USE_LOGGING
				if (t->mon && MON_CB(task_assign)) {
					MON_CB(task_assign)(t->mon, wp->mon);
				}
#endif
				break;
			default:
				assert(0);
				break;
			}
		}
	} while (!wp->terminate);
	LpelTaskDestroy(wp->current_task);
	/* cleanup task context marked for deletion */
}


static void addFreeWrapper(workerctx_t *wp) {
	assert (!LpelMailboxHasIncoming(wp->mailbox) && wp->terminate);
	PRODLOCK_LOCK(&lockwrappers);
	wp->next = freewrappers;
	freewrappers = wp;
	PRODLOCK_UNLOCK(&lockwrappers);
}

static workerctx_t *getFreeWrapper(){
	if (freewrappers == NULL)
		return NULL;
	workerctx_t *w;
	PRODLOCK_LOCK(&lockwrappers);
	if (freewrappers == NULL)
		w = NULL;
	else {
		w = freewrappers;
		freewrappers = w->next;
		w->next = NULL;
	}
	PRODLOCK_UNLOCK(&lockwrappers);
	return w;
}




void *WrapperThread(void *arg)
{

	workerctx_t *wp = (workerctx_t *)arg;

#ifdef HAVE___THREAD
	workerctx_cur = wp;
#else /* HAVE___THREAD */
	/* set pointer to worker context as TSD */
	pthread_setspecific(workerctx_key, wp);
#endif /* HAVE___THREAD */

#ifdef USE_MCTX_PCL
	assert(0 == co_thread_init());
	wp->mctx = co_current();
#endif

	LpelThreadAssign(wp->wid);
	WrapperLoop(wp);

	addFreeWrapper(wp);

#ifdef USE_MCTX_PCL
	co_thread_cleanup();
#endif
	return NULL;
}

workerctx_t *LpelCreateWrapperContext(int wid) {
	workerctx_t *wp = getFreeWrapper();
	if (wp == NULL) {
		wp = (workerctx_t *) malloc(sizeof(workerctx_t));
		/* mailbox */
			wp->mailbox = LpelMailboxCreate();
			wp->free_sd = NULL;
			wp->free_stream = NULL;
			wp->next = NULL;
	}
	wp->wid = wid;
	wp->terminate = 0;
	/* Wrapper is excluded from scheduling module */
	wp->current_task = NULL;
	wp->mon = NULL;

	/* taskqueue of free tasks */
	(void) pthread_create(&wp->thread, NULL, WrapperThread, wp);
	(void) pthread_detach(wp->thread);
	return wp;
}



/** return the total number of workers, including master */
int LpelWorkerCount(void)
{
  return num_workers;
}


/*******************************************************************************
 * WORKER FUNCTION
 ******************************************************************************/

void LpelWorkerBroadcast(workermsg_t *msg)
{
	int i;
	mailbox_t *wmb;

	for(i=0; i<num_workers; i++) {
		wmb = workermbs[i];
		/* send */
		LpelMailboxSend(wmb, msg);    
	}
}



static void WorkerLoop(workerctx_t *wc)
{
	WORKER_DBG("start worker %d\n", wc->wid);

  lpel_task_t *t = NULL;
  requestTask(wc);		// ask for the first time

  workermsg_t msg;
  do {
  	  LpelMailboxRecv(wc->mailbox, &msg);

  	  switch(msg.type) {
  	  case WORKER_MSG_ASSIGN:
  	  	t = msg.body.task;
  	  	WORKER_DBG("worker %d: get task %d\n", wc->wid, t->uid);
  	  	assert(t->state == TASK_READY);
  	  	t->worker_context = wc;
  	  	wc->current_task = t;

#ifdef USE_LOGGING
  	  	if (wc->mon && MON_CB(worker_waitstop)) {
  	  		MON_CB(worker_waitstop)(wc->mon);
  	  	}
  	  	if (t->mon && MON_CB(task_assign)) {
  	  		MON_CB(task_assign)(t->mon, wc->mon);
  	  	}
#endif
  	  	mctx_switch(&wc->mctx, &t->mctx);
  	  	//task return here
  	  	assert(t->state != TASK_RUNNING);
//  	  	if (t->state != TASK_ZOMBIE) {
  	  	wc->current_task = NULL;
  	  		t->worker_context = NULL;
  	  		returnTask(t);
//  	  	} else
//  	  		LpelTaskDestroy(t);		// if task finish, destroy it and not return to master
  	  	break;
  	  case WORKER_MSG_TERMINATE:
  	  	wc->terminate = 1;
  	  	break;
  	  default:
  	  	assert(0);
  	  	break;
  	  }
  	  // reach here --> message request for task has been sent
  } while (!(wc->terminate) );
}


void *WorkerThread(void *arg)
{
  workerctx_t *wc = (workerctx_t *)arg;

#ifdef HAVE___THREAD
  workerctx_cur = wc;
#else /* HAVE___THREAD */
  /* set pointer to worker context as TSD */
  pthread_setspecific(workerctx_key, wc);
#endif /* HAVE___THREAD */


//FIXME
#ifdef USE_MCTX_PCL
  assert(0 == co_thread_init());
  wc->mctx = co_current();
#endif

  wc->terminate = 0;
  wc->current_task = NULL;
	LpelThreadAssign(wc->wid + 1);		// 0 is for the master
	WorkerLoop(wc);

#ifdef USE_LOGGING
  /* cleanup monitoring */
  if (wc->mon && MON_CB(worker_destroy)) {
    MON_CB(worker_destroy)(wc->mon);
  }
#endif

#ifdef USE_MCTX_PCL
  co_thread_cleanup();
#endif
  return NULL;
}




workerctx_t *LpelWorkerSelf(void){
#ifdef HAVE___THREAD
  return workerctx_cur;
#else /* HAVE___THREAD */
  return (workerctx_t *) pthread_getspecific(workerctx_key);
#endif /* HAVE___THREAD */
}


lpel_task_t *LpelWorkerCurrentTask(void)
{
	 workerctx_t *w = LpelWorkerSelf();
	  /* It is quite a common bug to call LpelWorkerCurrentTask() from a non-task context.
	   * Provide an assertion error instead of just segfaulting on a null dereference. */
	  assert(w && "Currently not in an LPEL worker context!");
	  return w->current_task;
}


/******************************************
 * TASK RELATED FUNCTIONS
 ******************************************/

void LpelWorkerTaskExit(lpel_task_t *t) {
	workerctx_t *wc = t->worker_context;
	WORKER_DBG("worker %d: task %d exit\n", wc->wid, t->uid);
	if (wc->wid >= 0) {
		requestTask(wc);	// FIXME: should have requested before
		wc->current_task = NULL;
	}
	else
		wc->terminate = 1;		// wrapper: terminate

	mctx_switch(&t->mctx, &wc->mctx);		// switch back to the worker
}


void LpelWorkerTaskBlock(lpel_task_t *t){
	workerctx_t *wc = t->worker_context;
	if (wc->wid < 0) {	//wrapper
			wc->current_task = NULL;
	} else {
		WORKER_DBG("worker %d: block task %d\n", wc->wid, t->uid);
		//sendUpdatePrior(t);		//update prior for neighbor
		requestTask(wc);
	}
	wc->current_task = NULL;
	mctx_switch(&t->mctx, &wc->mctx);		// switch back to the worker/wrapper
}

void LpelWorkerTaskYield(lpel_task_t *t){
	workerctx_t *wc = t->worker_context;
	if (wc->wid < 0) {	//wrapper
			WORKER_DBG("wrapper: task %d yields\n", t->uid);
	}
	else {
		//sendUpdatePrior(t);		//update prior for neighbor
		requestTask(wc);
		WORKER_DBG("worker %d: return task %d\n", wc->wid, t->uid);
		wc->current_task = NULL;
	}
	mctx_switch(&t->mctx, &wc->mctx);		// switch back to the worker/wrapper
}

void LpelWorkerTaskWakeup(lpel_task_t *t) {
	workerctx_t *wc = t->worker_context;
	WORKER_DBG("worker %d: send wake up task %d\n", LpelWorkerSelf()->wid, t->uid);
	if (wc == NULL)
		sendWakeup(mastermb, t);
	else {
		if (wc->wid < 0)
			sendWakeup(wc->mailbox, t);
		else
			sendWakeup(mastermb, t);
	}
}



/******************************************
 * STREAM RELATED FUNCTIONS
 ******************************************/
void LpelWorkerPutStream(workerctx_t *wc, lpel_stream_t *s) {
	if (wc->free_stream == NULL) {
		wc->free_stream = s;
		s->next = NULL;
	} else {
		s->next = wc->free_stream;
		wc->free_stream = s;
	}
}

lpel_stream_t *LpelWorkerGetStream() {
	lpel_stream_t *tmp;
	workerctx_t *wc = LpelWorkerSelf();
	if (wc == NULL) {
		return NULL;
	}

	tmp = wc->free_stream;
	if (tmp) {
		wc->free_stream = tmp->next;
		tmp->next = NULL;
		assert(tmp->cons_sd == NULL && tmp->prod_sd == NULL);
	}
	return tmp;
}

void LpelWorkerPutSd(workerctx_t *wc, lpel_stream_desc_t *sd) {
	if (wc->free_sd == NULL) {
		wc->free_sd = sd;
		sd->next = NULL;
	} else {
		sd->next = wc->free_sd;
		wc->free_sd = sd;
	}
}

lpel_stream_desc_t *LpelWorkerGetSd(workerctx_t *wc) {
	lpel_stream_desc_t *tmp = wc->free_sd;
	if (tmp != NULL) {
		if (tmp->task == NULL && tmp->stream == NULL) {
			wc->free_sd = tmp->next;
			tmp->next = NULL;
			return tmp;
		} else {
			lpel_stream_desc_t *prev = tmp;
			tmp = tmp->next;
			while (tmp != NULL) {
				if (tmp->task == NULL && tmp->stream == NULL) {
					prev->next = tmp->next;
					tmp->next = NULL;
					return tmp;
				}
				prev = tmp;
				tmp = tmp->next;
			}
		}
	}
	return NULL;
}

void LpelWorkerDestroyStream(workerctx_t *wc) {
	lpel_stream_t *head = wc->free_stream;
	lpel_stream_t *tmp;
	while (head != NULL) {
		tmp = head->next;
		LpelStreamDestroy(head);
		head = tmp;
	}
}

void LpelWorkerDestroySd(workerctx_t *wc) {
	lpel_stream_desc_t *head = wc->free_sd;
	lpel_stream_desc_t *tmp;
	while (head != NULL) {
		tmp = head->next;
		free(head);
		head = tmp;
	}
}
