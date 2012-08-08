#ifndef _TASKQUEUE_H_
#define _TASKQUEUE_H_

#include <lpel.h>
#include "task.h"


struct taskqueue {
  lpel_task_t *head, *tail;
  unsigned int count;
};

void LpelTaskqueueInit( taskqueue_t *tq);

void LpelTaskqueuePushBack(  taskqueue_t *tq, lpel_task_t *t);
void LpelTaskqueuePushFront( taskqueue_t *tq, lpel_task_t *t);

lpel_task_t *LpelTaskqueuePopBack(  taskqueue_t *tq);
lpel_task_t *LpelTaskqueuePopFront( taskqueue_t *tq);


int LpelTaskqueueIterateRemove( taskqueue_t *tq,
    int  (*cond)( lpel_task_t*,void*),
    void (*action)(lpel_task_t*,void*),
    void *arg );

#endif /* _TASKQUEUE_H_ */
