#ifndef _WORKER_H_
#define _WORKER_H_


#include "task.h"


#include "workerctx.h"
#include "workermsg.h"





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
lpel_task_t *LpelWorkerCurrentTask(void);

void LpelWorkerSelfTaskExit(lpel_task_t *t);
void LpelWorkerSelfTaskYield(lpel_task_t *t);

#endif /* _WORKER_H_ */
