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


static int num_workers = -1;
static masterctx_t *master;
static int node_ID;
int node_ID;
static void *local;

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
        
	LpelMailboxInit();
	node_ID=SccGetNodeID();
  
  /* init key for thread specific data */
  initWorkerCtxKey();
      
	if (node_ID == master_ID) {
    /*create mailbox first as it uses special malloc*/
    mailbox_t *mbox =  LpelMailboxCreate(node_ID);

    /** create master */
    master = (masterctx_t *) malloc(sizeof(masterctx_t));         
    master->mailbox = mbox;
    master->ready_tasks = LpelTaskqueueInit ();
    master->num_workers = num_workers;
    /*master do not hold context for workers*/
    master->workers = NULL;

    /* allocate waiting table */
    master->waitworkers = (int *) malloc(num_workers * sizeof(int));
    for (i=0; i<num_workers; i++) {
      master->waitworkers[i] = 0;
    }
	} else {
    /*create single worker per core*/
    mailbox_t *mbox = LpelMailboxCreate(node_ID);
    /* allocate worker context table, but only for one worker */
    worker=(workerctx_t *) malloc(sizeof(workerctx_t));
    worker->wid=node_ID;

#ifdef USE_LOGGING
    if (MON_CB(worker_create)) {
      worker->mon = MON_CB(worker_create)(worker->wid);
    } else {
      worker->mon = NULL;
    }
#else
    worker->mon = NULL;
#endif
    /* mailbox */
    worker->mailbox = mbox;
    worker->free_sd = NULL;
    worker->free_stream = NULL;
	}
}

/*
 * clean up for both worker and master
 */
void LpelWorkersCleanup(void) {
  if (node_ID == master_ID) {
    /* wait for the master to finish */
    (void) pthread_join(master->thread, NULL);

    /* clean up master's mailbox */
    cleanupMasterMb();

    LpelMailboxDestroy(master->mailbox);
    LpelTaskqueueDestroy(master->ready_tasks);
    
   	/* free workers tables */
		free(master->waitworkers);
    
    /* clean up master's key and itself */
    cleanupWorkerCtxKey();
    free(master);
  } else {
    /* wait for the worker to finish */
		(void) pthread_join(worker->thread, NULL);
    
    /* cleanup the data structures */
    LpelMailboxDestroy(worker->mailbox);
		LpelWorkerDestroyStream(worker);
		LpelWorkerDestroySd(worker);
    
    /* clean up worker's key and itself */
    cleanupWorkerCtxKey();
		free(worker);
  }
}


/*
 * Spawn master and workers
 */
void LpelWorkersSpawn(void) {
  if (node_ID == master_ID) {
    /* master */
    (void) pthread_create(&master->thread, NULL, MasterThread, master); 	/* spawn joinable thread */
  } else {
    /* worker */
    (void) pthread_create(&worker->thread, NULL, WorkerThread, worker);
  }	
}


/*
 * Terminate master and workers
 */
void LpelWorkersTerminate(void) {
	workermsg_t msg;
	msg.type = WORKER_MSG_TERMINATE;
  if (node_ID == master_ID) {
    LpelWorkerBroadcast(&msg);
    //LpelMailboxSend(master->mailbox, &msg);
  }
}
