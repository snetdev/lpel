#ifndef _HRC_TASKQUEUE_H_
#define _HRC_TASKQUEUE_H_

#include "lpel_common.h"

//#define USE_STATIC_QUEUE
#define USE_HEAP_QUEUE

typedef struct taskqueue_t taskqueue_t;

taskqueue_t* LpelTaskqueueInit();

void LpelTaskqueuePush(taskqueue_t *tq, lpel_task_t *t);

lpel_task_t *LpelTaskqueuePop(taskqueue_t *tq);

void LpelTaskqueueDestroy(taskqueue_t *tq);

lpel_task_t *LpelTaskqueuePeek(taskqueue_t *tq);
void LpelTaskqueueOccupyTask(taskqueue_t *tq, lpel_task_t *t);

int LpelTaskqueueSize(taskqueue_t *tq);

void LpelTaskqueueUpdatePriority(taskqueue_t *tq, lpel_task_t *t, double np);

#endif /* _HRC_TASKQUEUE_H_ */
