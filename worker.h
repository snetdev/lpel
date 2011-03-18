#ifndef _WORKER_H_
#define _WORKER_H_

#include <pthread.h>
#include <pcl.h>

#include "lpel.h"

#include "arch/timing.h"

#include "scheduler.h"
#include "monitoring.h"
#include "taskqueue.h"

//#include "mailbox-lf.h"
#include "mailbox.h"




typedef struct {
  int node;
  int do_print_workerinfo;
} workercfg_t;



typedef struct {
  int wid; 
  pthread_t     thread;
  coroutine_t   mctx;
  unsigned int  loop;
  lpel_task_t  *predecessor;
  int           terminate;
  timing_t      wait_time;
  unsigned int  wait_cnt;
  //taskqueue_t   free_tasks;
  mailbox_t     mailbox;
  schedctx_t   *sched;
  lpel_task_t  *wraptask;
  monitoring_t *mon;
  char          padding[64];
} workerctx_t;



void _LpelWorkerInit( int size, workercfg_t *cfg);
void _LpelWorkerCleanup( void);
void _LpelWorkerTaskWakeup( lpel_task_t *by, lpel_task_t *whom);

void _LpelWorkerDispatcher( lpel_task_t *t);
void _LpelWorkerFinalizeTask( workerctx_t *wc);

void _LpelWorkerRunTask( lpel_task_t *t);
workerctx_t *_LpelWorkerId2Wc(int id);

#endif /* _WORKER_H_ */
