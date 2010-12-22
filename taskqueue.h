#ifndef _TASKQUEUE_H_
#define _TASKQUEUE_H_

#include "bool.h"

struct task;

typedef struct {
  struct task *head, *tail;
  unsigned int count;
} taskqueue_t;

void TaskqueueInit(taskqueue_t *tq);

void TaskqueuePushBack( taskqueue_t *tq, struct task *t);
void TaskqueuePushFront( taskqueue_t *tq, struct task *t);

struct task *TaskqueuePopBack(taskqueue_t *tq);
struct task *TaskqueuePopFront(taskqueue_t *tq);

#define TaskqueueEnqueue    TaskqueuePushBack
#define TaskqueueDequeue    TaskqueuePopFront

int TaskqueueIterateRemove(taskqueue_t *tq, 
    bool (*cond)(struct task*,void*), void (*action)(struct task*,void*), void *arg );

#endif /* _TASKQUEUE_H_ */
