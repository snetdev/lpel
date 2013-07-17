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

#define WORKER_PTR(i) (master->workers[(i)])
#define MASTER_PTR	master

//#define _USE_WORKER_DBG__

#ifdef _USE_WORKER_DBG__
#define WORKER_DBG printf
#else
#define WORKER_DBG	//
#endif


static int num_workers = -1;
static masterctx_t *master;

/**
 * Initialise worker globally
 *
 *
 * @param size    size of the worker set, i.e., the total number of workers including master
 */
void LpelWorkersInit(int size) {

	int i;
	assert(0 <= size);
	num_workers = size - 1;


	/** create master */
	master = (masterctx_t *) malloc(sizeof(masterctx_t));
	master->mailbox = LpelMailboxCreate();
	master->ready_tasks = LpelTaskqueueInit ();
	master->num_workers = num_workers;

	initWorkerCtxKey();

	/* allocate worker context table */
	master->workers = (workerctx_t **) malloc(num_workers * sizeof(workerctx_t*) );
	/* allocate waiting table */
	master->waitworkers = (int *) malloc(num_workers * sizeof(int));
	/* allocate worker contexts */
	for (i=0; i<num_workers; i++) {
		master->workers[i] = (workerctx_t *) malloc(sizeof(workerctx_t) );
		master->waitworkers[i] = 0;
	}

	/* prepare data structures */
	for(i=0; i<num_workers; i++) {
		workerctx_t *wc = WORKER_PTR(i);
		wc->wid = i;

#ifdef USE_LOGGING
		if (MON_CB(worker_create)) {
			wc->mon = MON_CB(worker_create)(wc->wid);
		} else {
			wc->mon = NULL;
		}
#else
	wc->mon = NULL;
#endif

	/* mailbox */
	wc->mailbox = LpelMailboxCreate();
	wc->free_sd = NULL;
	wc->free_stream = NULL;
	}

	/* free wrapper */
	initFreeWrappers();
}

/*
 * clean up for both worker and master
 */
void LpelWorkersCleanup(void) {
	int i;
	workerctx_t *wc;

	for(i=0; i<num_workers; i++) {
		wc = WORKER_PTR(i);
		/* wait for the worker to finish */
		(void) pthread_join(wc->thread, NULL);
	}
	/* wait for the master to finish */
	(void) pthread_join(master->thread, NULL);

	/* clean up master's mailbox */
	cleanupMasterMb();

	LpelMailboxDestroy(master->mailbox);
	LpelTaskqueueDestroy(master->ready_tasks);


	free(master);

	/* cleanup the data structures */
	for(i=0; i<num_workers; i++) {
		wc = WORKER_PTR(i);
		LpelMailboxDestroy(wc->mailbox);
		LpelWorkerDestroyStream(wc);
		LpelWorkerDestroySd(wc);
		free(wc);
	}

	/* free workers tables */
		free(master->workers);
		free(master->waitworkers);

		/* free wrappers */
		cleanupFreeWrappers();
		cleanupWorkerCtxKey();
}


/*
 * Spawn master and workers
 */
void LpelWorkersSpawn(void) {
	int i;
	/* master */
	(void) pthread_create(&master->thread, NULL, MasterThread, MASTER_PTR); 	/* spawn joinable thread */

	/* workers */
	for(i=0; i<num_workers; i++) {
		workerctx_t *wc = WORKER_PTR(i);
		(void) pthread_create(&wc->thread, NULL, WorkerThread, wc);
	}
}


/*
 * Terminate master and workers
 */
void LpelWorkersTerminate(void) {
	workermsg_t msg;
	msg.type = WORKER_MSG_TERMINATE;
	LpelMailboxSend(MASTER_PTR->mailbox, &msg);
}
