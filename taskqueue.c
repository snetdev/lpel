

#include <malloc.h>

#include "bool.h"

/**
 * A doubly linked list for task queues
 * - append at tail, remove from head (FIFO)
 * - insert at arbitrary point depending on priority
 *   (insertion sort, O(n) in this case)
 * - (sort list, if it is possible that priorities change
 *   dynamically, tbd. if required -> method??)
 */

typedef struct taskqueue taskqueue_t;


/**
 * Invariants:
 *   (head != NULL)   =>  (head->prev == NULL) AND
 *   (tail != NULL)   =>  (tail->next == NULL) AND
 *   (head == NULL)  <=>  (tail == NULL)
 */

struct taskqueue {
  task_t *head;
  task_t *tail;
  unsigned int count;

  // MUTEX
  // CONDVAR
};



/**
 * Create a new taskqueue
 */
taskqueue_t *TaskqueueCreate(void)
{
  taskqueue_t *tq = (taskqueue_t *) malloc( sizeof(taskqueue_t) );
  tq->head = NULL;
  tq->tail = NULL;
  tq->count = 0;

  // INIT MUTEX
  // INIT CONDVAR
}


/**
 * Destroy the taskqueue
 */
void TaskqueueDestroy(taskqueue_t *tq)
{
  // DESTROY MUTEX
  // DESTROY CONDVAR

  free(tq);
  tq = NULL;
}


/**
 * Append a task into a taskqueue (at tail)
 * Preconditions:
 *   (tq != NULL) AND (t != NULL) AND
 *   (t->next == t->prev == NULL)
 *
 * which means, that t must not be part of another taskqueue
 */
void TaskqueueAppend(taskqueue_t *tq, task_t *t)
{
  // LOCK

  if ( tq->head == NULL ) {
    tq->head = t;
    /* t->prev = NULL; is precond */
  } else {
    tq->tail->next = t;
    t->prev = tail;
  }
  tq->tail = t;
  /* t->next = NULL; is precond */

  /* increment task count */
  tq->count++;

  // SIGNAL COND

  // UNLOCK
}


/**
 * Removes the head from a taskqueue and returns it
 *
 * Preconditions:
 * - MUTEX is aquired
 * - tq->head != NULL (i.e., taskqueue not empty)
 */
static task_t *TaskqueueRemoveHead(taskqueue_t *tq)
{
  task_t *t = tq->head;
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
  return t;
}


/**
 * Remove a task from a taskqueue (from head)
 * - BLOCKING!
 */
task_t *TaskqueueRemove(taskqueue_t *tq)
{
  task_t *t;

  // LOCK
  while (tq->head != NULL) {
    //WAIT COND
  }
  t = TaskqueueRemoveHead(tq);
  // UNLOCK

  return t;
}


/**
 * Remove a task from a taskqueue (from head)
 * - NON-BLOCKING!
 * - if no task is in queue, simply returns NULL
 */
task_t *TaskqueueTryRemove(taskqueue_t *tq)
{
  task_t *t = NULL;

  // LOCK
  if ( tq->head != NULL ) {
    t = TaskqueueRemoveHead(tq);
  }
  // UNLOCK

  return t;
}
