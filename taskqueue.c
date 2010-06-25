
#include <malloc.h>
#include "lpel_p.h"


/**
 * A simple doubly linked list for task queues
 * - append at tail, remove from head (FIFO)
 *
 * Invariants:
 *   (head != NULL)   =>  (head->prev == NULL) AND
 *   (tail != NULL)   =>  (tail->next == NULL) AND
 *   (head == NULL)  <=>  (tail == NULL)
 */
struct taskqueue {
  task_t *head;
  task_t *tail;
  unsigned int count;
};



/**
 * Create a new taskqueue
 */
void TaskqueueInit(taskqueue_t *tq)
{
  tq->head = NULL;
  tq->tail = NULL;
  tq->count = 0;
}

/**
 * Get the number of tasks in the queue
 */
unsigned int TaskqueueCount(taskqueue_t *tq)
{
  return tq->count;
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


/**
 * Removes the head from a taskqueue and returns it
 *
 * Preconditions:
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
 * - if no task is in queue, simply returns NULL
 */
task_t *TaskqueueRemove(taskqueue_t *tq)
{
  task_t *t = NULL;

  if ( tq->head != NULL ) {
    t = TaskqueueRemoveHead(tq);
  }

  return t;
}

void TaskqueueIterateRemove(taskqueue_t *tq, 
  bool (*cond)(task_t*), void (*action)(task_t*) )
{
  task_t *cur = tq->head;
  while (cur != NULL) {
    /* check condition */
    if ( cond(cur) ) {
      task_t *p = cur;

      /* relink the queue */
      if (cur->prev != NULL) { cur->prev->next = cur->next; }
      if (cur->next != NULL) { cur->next->prev = cur->prev; }
      cur = cur->next;
    
      p->prev = NULL;
      p->next = NULL;
      /* do action */
      action(p);
    } else {
      cur = cur->next;
    }
  }


}
