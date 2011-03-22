#ifndef _WORKER_H_
#define _WORKER_H_

#include <pthread.h>
#include <pcl.h>


#include "arch/timing.h"

#include "task.h"
#include "scheduler.h"
#include "taskqueue.h"

//#include "mailbox-lf.h"
#include "mailbox.h"




typedef struct {
  int node;
  int do_print_workerinfo;
} workercfg_t;


typedef struct workerctx_t workerctx_t;





void LpelWorkerInit( int size, workercfg_t *cfg);
void LpelWorkerCleanup( void);
void LpelWorkerTaskWakeup( lpel_task_t *by, lpel_task_t *whom);

void LpelWorkerDispatcher( lpel_task_t *t);
void LpelWorkerFinalizeTask( workerctx_t *wc);

void LpelWorkerRunTask( lpel_task_t *t);
void LpelWorkerSpawn(void);
void LpelWorkerTerminate(void);
workerctx_t *LpelWorkerGetContext(int id);

#endif /* _WORKER_H_ */
