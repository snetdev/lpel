/*
 * hrc_worker_init.c
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


//#define _USE_WORKER_DBG__

#ifdef _USE_WORKER_DBG__
#define WORKER_DBG printf
#else
#define WORKER_DBG	//
#endif

static void cleanupMasterMb();

static int num_workers = -1;
static masterctx_t *master;
static workerctx_t **workers;
/**
 * Initialise worker globally
 *
 *
 * @param size    size of the worker set, i.e., the total number of workers including master
 */
void LpelWorkersInit(lpel_config_t *conf) {

	int i;
	int size = conf->num_workers;
	assert(0 <= size);
	num_workers = size - 1;

	/* set up for task priority */
	LpelTaskPrioInit(&conf->prio_config);

	/** create master */
	master = (masterctx_t *) malloc(sizeof(masterctx_t));
	master->mailbox = LpelMailboxCreate();
	master->ready_tasks = LpelTaskqueueInit();
	master->num_workers = num_workers;

	/* allocate worker context table */
	workers = (workerctx_t **) malloc(num_workers * sizeof(workerctx_t*) );
	/* allocate waiting table */
	master->waitworkers = (int *) malloc(num_workers * sizeof(int));
	/* allocate worker contexts */
	for (i=0; i<num_workers; i++) {
		workers[i] = (workerctx_t *) malloc(sizeof(workerctx_t) );
		master->waitworkers[i] = 0;
    
		workers[i]->wid = i;

#ifdef USE_LOGGING
		if (MON_CB(worker_create)) {
			workers[i]->mon = MON_CB(worker_create)(workers[i]->wid);
		} else {
			workers[i]->mon = NULL;
		}
#else
	workers[i]->mon = NULL;
#endif

	/* mailbox */
	workers[i]->mailbox = LpelMailboxCreate();
	workers[i]->free_sd = NULL;
	workers[i]->free_stream = NULL;
	}

	/* local variables used in worker operations */
	initLocalVar(num_workers);


}


void setupMailbox(mailbox_t **mastermb, mailbox_t **workermbs) {
  *mastermb = master->mailbox;
  int i;
  for (i = 0; i < num_workers; i++)
    workermbs[i] = workers[i]->mailbox;
}

/*
 * clean up for both worker and master
 */
void LpelWorkersCleanup(void) {
	int i;
	workerctx_t *wc;

	for(i=0; i<num_workers; i++) {
		wc = workers[i];
		/* wait for the worker to finish */
		(void) pthread_join(wc->thread, NULL);
	}
	/* wait for the master to finish */
	(void) pthread_join(master->thread, NULL);

	/* clean up master's mailbox */
	cleanupMasterMb();

	LpelMailboxDestroy(master->mailbox);
	LpelTaskqueueDestroy(master->ready_tasks);


	/* cleanup the data structures */
	for(i=0; i<num_workers; i++) {
		wc = workers[i];
		LpelMailboxDestroy(wc->mailbox);
		LpelWorkerDestroyStream(wc);
		LpelWorkerDestroySd(wc);
		free(wc);
	}

	/* free workers tables */
		free(workers);
		free(master->waitworkers);

		/* clean up local vars used in worker operations */
		cleanupLocalVar();
		    
    free(master);
}


/*
 * Spawn master and workers
 */
void LpelWorkersSpawn(void) {
	int i;
	/* master */
	(void) pthread_create(&master->thread, NULL, MasterThread, master); 	/* spawn joinable thread */

	/* workers */
	for(i=0; i<num_workers; i++) {
		workerctx_t *wc = workers[i];
		(void) pthread_create(&wc->thread, NULL, WorkerThread, wc);
	}
}


/*
 * Terminate master and workers
 */
void LpelWorkersTerminate(void) {
	workermsg_t msg;
	msg.type = WORKER_MSG_TERMINATE;
	LpelMailboxSend(master->mailbox, &msg);
}

/************************ Private functions ***********************************/
/*
 * clean up master's mailbox before terminating master
 * last messages including: task request from worker, and return zombie task
 */
static void cleanupMasterMb() {
	workermsg_t msg;
	lpel_task_t *t;
	while (LpelMailboxHasIncoming(master->mailbox)) {
		LpelMailboxRecv(master->mailbox, &msg);
		switch(msg.type) {
		case WORKER_MSG_REQUEST:
			break;
		case WORKER_MSG_RETURN:
			t = msg.body.task;
			WORKER_DBG("master: get returned task %d\n", t->uid);
	    assert(t->state == TASK_ZOMBIE);
			LpelTaskDestroy(t);
			break;
		default:
			assert(0);
			break;
		}
	}
}
