#ifndef _WORKER_H_
#define _WORKER_H_

#include <pthread.h>
#include <lpel.h>

#include "arch/mctx.h"
#include "arch/atomic.h"
#include "task.h"
#include "scheduler.h"
#include "mailbox.h"


typedef struct workerctx_t {
  int wid;
  pthread_t     thread;
  mctx_t        mctx;
  int           terminate;
  unsigned int  num_tasks;
  lpel_task_t  *current_task;
  lpel_task_t  *marked_del;
  mon_worker_t *mon;
  mailbox_t    *mailbox;
  schedctx_t   *sched;
  lpel_task_t  *wraptask;
  atomic_voidptr free_tasks;
  worker_placement_t *placement_data;
#ifdef MEASUREMENTS
  long max_time;
  long min_time;
  long total_tasks;
#endif
} workerctx_t;

extern workerctx_t **workers;


#ifdef LPEL_DEBUG_WORKER
/* use the debug callback if available to print a debug message */
#define WORKER_DBGMSG(wc,...) do {\
  if ((wc)->mon && MON_CB(worker_debug)) { \
    MON_CB(worker_debug)( (wc)->mon, ##__VA_ARGS__ ); \
  }} while(0)

#else /* LPEL_DEBUG_WORKER */
#define WORKER_DBGMSG(wc,...) /*NOP*/
#endif /* LPEL_DEBUG_WORKER */





void LpelWorkerInit( int size);
void LpelWorkerCleanup( void);
void LpelWorkerRunTask( lpel_task_t *t);


void LpelWorkerDispatcher( lpel_task_t *t);
void LpelWorkerSpawn(void);
void LpelWorkerTaskWakeup( lpel_task_t *by, lpel_task_t *whom);
void LpelWorkerTaskWakeupLocal( workerctx_t *wc, lpel_task_t *task);
void LpelWorkerTerminate(void);
void LpelWorkerBroadcast(workermsg_t *msg);
workerctx_t *LpelWorkerGetContext(int id);
workerctx_t *LpelWorkerSelf(void);
lpel_task_t *LpelWorkerCurrentTask(void);

void LpelCollectTask(workerctx_t* wc, lpel_task_t* t);
void LpelWorkerSelfTaskExit(lpel_task_t *t);
void LpelWorkerSelfTaskYield(lpel_task_t *t);

workerctx_t **LpelWorkerGetWorkers();
#endif /* _WORKER_H_ */
