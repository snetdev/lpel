#ifndef _TASKQUEUE_H_
#define _TASKQUEUE_H_

#include "lpel_name.h"
#include "task.h"


typedef struct {
  lpel_task_t *head, *tail;
  unsigned int count;
} taskqueue_t;

void LPEL_EXPORT(TaskqueueInit)( taskqueue_t *tq);

void LPEL_EXPORT(TaskqueuePushBack)(  taskqueue_t *tq, lpel_task_t *t);
void LPEL_EXPORT(TaskqueuePushFront)( taskqueue_t *tq, lpel_task_t *t);

lpel_task_t *LPEL_EXPORT(TaskqueuePopBack)(  taskqueue_t *tq);
lpel_task_t *LPEL_EXPORT(TaskqueuePopFront)( taskqueue_t *tq);


int LPEL_EXPORT(TaskqueueIterateRemove)( taskqueue_t *tq,
    int  (*cond)( lpel_task_t*,void*),
    void (*action)(lpel_task_t*,void*),
    void *arg );

#endif /* _TASKQUEUE_H_ */
