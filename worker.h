#ifndef _WORKER_H_
#define _WORKER_H_


#include "lpel_name.h"
#include "task.h"


#include "workerctx.h"





void LPEL_EXPORT(WorkerInit)( int size);
void LPEL_EXPORT(WorkerCleanup)( void);
void LPEL_EXPORT(WorkerRunTask)( lpel_task_t *t);


void LPEL_EXPORT(WorkerDispatcher)( lpel_task_t *t);
void LPEL_EXPORT(WorkerSpawn)(void);
void LPEL_EXPORT(WorkerTaskWakeup)( lpel_task_t *by, lpel_task_t *whom);
void LPEL_EXPORT(WorkerTerminate)(void);
workerctx_t *LPEL_EXPORT(WorkerGetContext)(int id);
lpel_task_t *LPEL_EXPORT(WorkerCurrentTask)(void);

void LPEL_EXPORT(WorkerSelfTaskExit)(lpel_task_t *t);
void LPEL_EXPORT(WorkerSelfTaskYield)(lpel_task_t *t);

#endif /* _WORKER_H_ */
