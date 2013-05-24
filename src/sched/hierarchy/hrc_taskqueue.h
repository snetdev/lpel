#ifndef _HRC_TASKQUEUE_H_
#define _HRC_TASKQUEUE_H_

#include "lpel_common.h"


typedef struct taskqueue_t taskqueue_t;

taskqueue_t *LpelTaskqueueInit();

void LpelTaskqueuePush(  taskqueue_t *tq, lpel_task_t *t);

lpel_task_t *LpelTaskqueuePop( taskqueue_t *tq);


int LpelTaskqueueIterateRemove( taskqueue_t *tq,
    int  (*cond)( lpel_task_t*,void*),
    void (*action)(lpel_task_t*,void*),
    void *arg );

void LpelTaskqueueDestroy(taskqueue_t *tq);

lpel_task_t *LpelTaskqueuePeek( taskqueue_t *tq);
int LpelTaskqueueSize(taskqueue_t *tq);

void LpelTaskqueueUpdatePriority(taskqueue_t *tq, lpel_task_t *t, double np);

#endif /* _HRC_TASKQUEUE_H_ */
