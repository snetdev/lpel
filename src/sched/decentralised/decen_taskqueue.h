#ifndef _DECEN_TASKQUEUE_H_
#define _DECEN_TASKQUEUE_H_

#include "decen_task.h"

typedef struct taskqueue_t taskqueue_t;
struct taskqueue_t {
  lpel_task_t *head, *tail;
  unsigned int count;
};

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

void LpelTaskqueuePushFront( taskqueue_t *tq, lpel_task_t *t);
lpel_task_t *LpelTaskqueuePopBack(  taskqueue_t *tq);

#endif 	/* _DECEN_TASKQUEUE_H */

