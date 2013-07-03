
#include <stdlib.h>
#include <assert.h>


#include "decen_taskqueue.h"
#include "decen_task.h"

/**
 * A simple doubly linked list for task queues
 * - enqueue at tail, dequeue from head (FIFO)
 *
 * Invariants:
 *   (head != NULL)   =>  (head->prev == NULL) AND
 *   (tail != NULL)   =>  (tail->next == NULL) AND
 *   (head == NULL)  <=>  (tail == NULL)
 */


/**
 * Initialise a taskqueue
 */
taskqueue_t *LpelTaskqueueInit()
{
	taskqueue_t *tq = (taskqueue_t *)malloc(sizeof(taskqueue_t));
  tq->head = NULL;
  tq->tail = NULL;
  tq->count = 0;
  return tq;
}


/**
 * Enqueue a task at the tail
 *
 */
void LpelTaskqueuePush(taskqueue_t *tq, lpel_task_t *t)
{
  assert( t->prev==NULL && t->next==NULL );

  if ( tq->head == NULL ) {
    tq->head = t;
    /* t->prev = NULL; is precond */
  } else {
    tq->tail->next = t;
    t->prev = tq->tail;
  }
  tq->tail = t;
  /* t->next = NULL; is precond */

  /* increment task count */
  tq->count++;
}


/*
 *
 * Enqueue a task at the head
 *
 */
void LpelTaskqueuePushFront(taskqueue_t *tq, lpel_task_t *t)
{
  assert( t->prev==NULL && t->next==NULL );

  if ( tq->tail == NULL ) {
    tq->tail = t;
    /* t->next = NULL; is precond */
  } else {
    tq->head->prev = t;
    t->next = tq->head;
  }
  tq->head = t;
  /* t->prev = NULL; is precond */

  /* increment task count */
  tq->count++;
}



/**
 * Dequeue a task from the head
 *
 * @return NULL if taskqueue is empty
 */
lpel_task_t *LpelTaskqueuePop(taskqueue_t *tq)
{
  lpel_task_t *t;

  if ( tq->head == NULL ) return NULL;

  t = tq->head;
  /* t->prev == NULL by invariant */
  if ( t->next == NULL ) {
    /* t is the single element in the list */
    /* t->next == t->prev == NULL by invariant */
    tq->head = NULL;
    tq->tail = NULL;
  } else {
    tq->head = t->next;
    tq->head->prev = NULL;
    t->next = NULL;
  }
  /* decrement task count */
  tq->count--;
  assert( t!=NULL || tq->count==0);
  assert(t->next==NULL && t->prev==NULL);
  return t;
}


/**
 * Dequeue a task from the tail
 *
 * @return NULL if taskqueue is empty
 */
lpel_task_t *LpelTaskqueuePopBack(taskqueue_t *tq)
{
  lpel_task_t *t;

  if ( tq->tail == NULL ) return NULL;

  t = tq->tail;
  /* t->next == NULL by invariant */
  if ( t->prev == NULL ) {
    /* t is the single element in the list */
    /* t->next == t->prev == NULL by invariant */
    tq->tail = NULL;
    tq->head = NULL;
  } else {
    tq->tail = t->prev;
    tq->tail->next = NULL;
    t->prev = NULL;
  }
  /* decrement task count */
  tq->count--;
  assert( t!=NULL || tq->count==0);
  assert(t->next==NULL && t->prev==NULL);
  return t;
}




/**
 * Iterates once through the taskqueue (starting at head),
 * for each task:
 *
 *  - if a condition is satisfied, unlink the task
 *  - perform an action on the task
 *
 * @param tq      taskqueue
 * @param cond    callback for querying the condition
 * @param action  callback for the action after unlinking
 * @param arg     argument (context) for the callback functions
 */
int LpelTaskqueueIterateRemove(taskqueue_t *tq,
  int (*cond)(lpel_task_t*, void*), void (*action)(lpel_task_t*, void*), void *arg )
{
  int cnt_removed = 0;
  lpel_task_t *cur = tq->head;
  while (cur != NULL) {
    /* check condition */
    if ( cond(cur, arg) ) {
      lpel_task_t *p = cur;

      /* relink the queue */
      if (cur->prev != NULL) {
        cur->prev->next = cur->next;
      } else {
        /* is head */
        tq->head = cur->next;
      }
      if (cur->next != NULL) {
        cur->next->prev = cur->prev;
      } else {
        /* is tail */
        tq->tail = cur->prev;
      }
      cur = cur->next;

      p->prev = NULL;
      p->next = NULL;
      /* decrement task count */
      tq->count--;
      cnt_removed++;
      /* do action */
      action(p, arg);
    } else {
      cur = cur->next;
    }
  }
  return cnt_removed;
}



void LpelTaskqueueDestroy(taskqueue_t *tq){
	assert(tq->head == NULL && tq->tail == NULL);
  free(tq);
}
