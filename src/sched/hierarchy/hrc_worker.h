#ifndef _HRC_WORKER_H_
#define _HRC_WORKER_H_

#include <pthread.h>
#include <hrc_lpel.h>
#include "lpel_main.h"
#include "arch/mctx.h"
#include "hrc_task.h"
#include "mailbox.h"
#include "hrc_taskqueue.h"
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


typedef struct workerctx_t {
  int wid;
  pthread_t     thread;
  mctx_t        mctx;
  int           terminate;
  lpel_task_t  *current_task;
  mon_worker_t *mon;
  mailbox_t    *mailbox;
  char          padding[64];
} workerctx_t;


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
workerctx_t *LpelWorkerSelf(void);
lpel_task_t *LpelWorkerCurrentTask(void);

void LpelWorkerTaskWakeup( lpel_task_t *whom);
void LpelWorkerTaskExit(lpel_task_t *t);
void LpelWorkerTaskYield(lpel_task_t *t);
void LpelWorkerTaskBlock(lpel_task_t *t);
void LpelWorkerRunTask( lpel_task_t *t);

void LpelWorkerBroadcast(workermsg_t *msg);

#endif /* _HRC_WORKER_H_ */
