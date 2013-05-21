#ifndef _HRC_WORKER_H_
#define _HRC_WORKER_H_

#include <pthread.h>
#include <hrc_lpel.h>

#include "arch/mctx.h"
#include "worker.h"
#include "hrc_task.h"
#include "mailbox.h"
#include "taskqueue.h"
#include "hrc_stream.h"

//#define _USE_DBG__

#ifdef _USE_DBG__
#define PRT_DBG printf
#else
#define PRT_DBG	//
#endif


#define  WORKER_MSG_TERMINATE 	1
#define  WORKER_MSG_WAKEUP			2		// send to both master and wrapper
#define  WORKER_MSG_ASSIGN			3
#define  WORKER_MSG_REQUEST			4		// worker request task
#define  WORKER_MSG_RETURN			5		// worker return tasks
#define  WORKER_MSG_PRIORITY		6		// sent to master only


struct workerctx_t {
  int wid;
  pthread_t     thread;
  mctx_t        mctx;
  int           terminate;
  lpel_task_t  *current_task;
  mon_worker_t *mon;
  mailbox_t    *mailbox;
  char          padding[64];
  lpel_stream_t *free_stream;			/* list of free stream */
  lpel_stream_desc_t *free_sd;		/* list of free stream desc */
};


typedef struct masterctx_t {
  pthread_t     thread;
  mctx_t        mctx;
  int           terminate;
  //mon_worker_t *mon; // FIXME
  mailbox_t    *mailbox;
  taskqueue_t  *ready_tasks;
  char          padding[64];
} masterctx_t;


workerctx_t *LpelCreateWrapperContext(int wid);		// can be wrapper or source/sink
void LpelWorkerTaskBlock(lpel_task_t *t);
void LpelWorkerTaskWakeup( lpel_task_t *t);
int LpelWorkerIsWrapper(workerctx_t *wc);


/* put and get free stream */
void LpelWorkerPutStream(workerctx_t *wc, lpel_stream_t *s);
lpel_stream_t *LpelWorkerGetStream();

/* put and get free stream desc*/
void LpelWorkerPutSd(workerctx_t *wc, lpel_stream_desc_t *s);
lpel_stream_desc_t *LpelWorkerGetSd(workerctx_t *wc);

/* destroy list of free stream and stream desc when terminate worker */
void LpelWorkerDestroyStream(workerctx_t *wc);
void LpelWorkerDestroySd(workerctx_t *wc);

#endif /* _HRC_WORKER_H_ */
