#ifndef _TASKQUEUE_H_
#define _TASKQUEUE_H_

#include "lpel_name.h"
#include "task.h"


typedef struct {
  lpel_task_t *head, *tail;
  unsigned int count;
} taskqueue_t;

void LPEL_FUNC(TaskqueueInit)( taskqueue_t *tq);

void LPEL_FUNC(TaskqueuePushBack)(  taskqueue_t *tq, lpel_task_t *t);
void LPEL_FUNC(TaskqueuePushFront)( taskqueue_t *tq, lpel_task_t *t);

lpel_task_t *LPEL_FUNC(TaskqueuePopBack)(  taskqueue_t *tq);
lpel_task_t *LPEL_FUNC(TaskqueuePopFront)( taskqueue_t *tq);


int LPEL_FUNC(TaskqueueIterateRemove)( taskqueue_t *tq,
    int  (*cond)( lpel_task_t*,void*),
    void (*action)(lpel_task_t*,void*),
    void *arg );

#endif /* _TASKQUEUE_H_ */
