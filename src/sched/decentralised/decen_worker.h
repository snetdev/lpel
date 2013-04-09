#ifndef _DECEN_WORKER_H_
#define _DECEN_WORKER_H_

#include "task.h"
#include "worker.h"
#include "decen_scheduler.h"


#define  WORKER_MSG_TERMINATE 	1
#define  WORKER_MSG_WAKEUP			2
#define  WORKER_MSG_ASSIGN			3
#define  WORKER_MSG_SPMDREQ			4
#define  WORKER_MSG_TASKMIG			5


struct workerctx_t {
  int wid;
  pthread_t     thread;
  mctx_t        mctx;
  int           terminate;
  unsigned int  num_tasks;
  //taskqueue_t   free_tasks;
  lpel_task_t  *current_task;
  lpel_task_t  *marked_del;
  mon_worker_t *mon;
  mailbox_t    *mailbox;
  schedctx_t   *sched;
  lpel_task_t  *wraptask;
  char          padding[64];
};


void LpelWorkerTaskWakeup( lpel_task_t *by, lpel_task_t *whom);
void LpelWorkerTaskWakeupLocal( workerctx_t *wc, lpel_task_t *task);

#endif /* _DECEN_WORKER_H */
