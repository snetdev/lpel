#ifndef _DECEN_WORKER_H_
#define _DECEN_WORKER_H_

#include "decen_scheduler.h"
#include <pthread.h>
#include <lpel_common.h>

#include "arch/mctx.h"
#include "decen_task.h"
#include "mailbox.h"


typedef struct workerctx_t workerctx_t;

#ifdef LPEL_DEBUG_WORKER
/* use the debug callback if available to print a debug message */
#define WORKER_DBGMSG(wc,...) do {\
  if ((wc)->mon && MON_CB(worker_debug)) { \
    MON_CB(worker_debug)( (wc)->mon, ##__VA_ARGS__ ); \
  }} while(0)

#else /* LPEL_DEBUG_WORKER */
#define WORKER_DBGMSG(wc,...) /*NOP*/
#endif /* LPEL_DEBUG_WORKER */


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
  lpel_task_t	 *migrated;
};

void LpelWorkerRunTask( lpel_task_t *t);
void LpelWorkerDispatcher( lpel_task_t *t);

void LpelWorkerBroadcast(workermsg_t *msg);
workerctx_t *LpelWorkerGetContext(int id);
workerctx_t *LpelWorkerSelf(void);
lpel_task_t *LpelWorkerCurrentTask(void);

void LpelWorkerSelfTaskExit(lpel_task_t *t);
void LpelWorkerSelfTaskYield(lpel_task_t *t);
void LpelWorkerTaskBlock(lpel_task_t *t);

void LpelWorkerTaskWakeup( lpel_task_t *by, lpel_task_t *whom);
void LpelWorkerTaskWakeupLocal( workerctx_t *wc, lpel_task_t *task);
void LpelWorkerSelfTaskMigrate(lpel_task_t *t, int target);

#endif /* _DECEN_WORKER_H */
