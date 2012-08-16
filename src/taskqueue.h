#ifndef _TASKQUEUE_H_
#define _TASKQUEUE_H_

#include "lpel.h"
#include "task.h"
#include "arch/atomic.h"

/**
 * A quick and dirty (but working) lock-free LIFO for
 * the task free list.
 * inspired from:
 * http://www.research.ibm.com/people/m/michael/RC23089.pdf
 */
#define LpelPushTask(ptr, top, task) \
{ \
    lpel_task_t *tmp; \
    do { \
        tmp = (lpel_task_t*) atomic_load(top); \
        task->ptr = tmp; \
    } while (!atomic_test_and_set(top, tmp, task)); \
}

#define LpelPopTask(ptr, top) \
({ \
    lpel_task_t *tmp; \
    lpel_task_t *next; \
    do { \
        tmp = (lpel_task_t*) atomic_load(top); \
        if (tmp == NULL) break; \
        next = tmp->ptr; \
    } while (!atomic_test_and_set(top, tmp, next)); \
    tmp; \
})



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
