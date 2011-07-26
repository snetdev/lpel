#ifndef _WORKER_H_
#define _WORKER_H_


#include "lpel_name.h"
#include "task.h"


#include "workerctx.h"





void LPEL_FUNC(WorkerInit)( int size);
void LPEL_FUNC(WorkerCleanup)( void);
void LPEL_FUNC(WorkerRunTask)( lpel_task_t *t);


void LPEL_FUNC(WorkerDispatcher)( lpel_task_t *t);
void LPEL_FUNC(WorkerSpawn)(void);
void LPEL_FUNC(WorkerTaskWakeup)( lpel_task_t *by, lpel_task_t *whom);
void LPEL_FUNC(WorkerTerminate)(void);
workerctx_t *LPEL_FUNC(WorkerGetContext)(int id);
lpel_task_t *LPEL_FUNC(WorkerCurrentTask)(void);

void LPEL_FUNC(WorkerSelfTaskExit)(lpel_task_t *t);
void LPEL_FUNC(WorkerSelfTaskYield)(lpel_task_t *t);

#endif /* _WORKER_H_ */
