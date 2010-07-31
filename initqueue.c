/**
 * Init-queue implementation using a variant of the two-lock queue of
 * Maged M. Michael and Michael L. Scott.
 *
 * Actually, only the Tail-lock is required, as only the owning worker
 * is dequeueing tasks from the head.
 *
 * Pseudocode can be found at:
 * http://www.cs.rochester.edu/research/synchronization/pseudocode/queues.html#tlq
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "initqueue.h"

/**
 * Initialise an init-queue
 */
void InitqueueInit(initqueue_t *iq)
{
  /* create a dummy node */
  task_t *dummy = (task_t *) calloc( 1, sizeof(task_t) );
  /* make it only node in the linked list */
  dummy->next = NULL;
  dummy->prev = NULL;

  /* both head and tail point to it */
  iq->head = dummy;
  iq->tail = dummy;

  pthread_mutex_init(&iq->lock_tail, NULL);
}

/**
 * Cleanup the initqueue
 */
void InitqueueCleanup(initqueue_t *iq)
{
  assert( iq->head->next == NULL ); /* queue empty */
  pthread_mutex_destroy(&iq->lock_tail);
}

/**
 * Enqueue a task at the tail
 * - enqueuers synchronize with the lock
 */
void InitqueueEnqueue(initqueue_t *iq, task_t *t)
{
  assert( t->next == NULL );
  /* aquire tail lock */
  pthread_mutex_lock(&iq->lock_tail);
  /* link task at the end of the linked list */
  iq->tail->next = t;
  /* swing tail to task */
  iq->tail = t;
  /* release tail lock */
  pthread_mutex_unlock(&iq->lock_tail);
}

/**
 * Dequeue a task from the head
 *
 * - supposed only to be called by the owning worker, hence
 *   compared to the original implementation, does not need a head-lock
 *
 * Note: The task returned will have a different
         address as it had when enqueued!
 *
 * @return the task from the head or NULL if queue is empty
 */
task_t *InitqueueDequeue(initqueue_t *iq)
{
  /* read head */
  task_t *node = iq->head;
  /* read next pointer */
  task_t *new_head = node->next;

  /* is queue empty? */
  if (new_head == NULL) return NULL;
  
  /* queue is not empty, copy values */
  memcpy(node, new_head, sizeof(task_t));
  node->next = NULL;

  /* swing head to next node (becomes new dummy) */
  iq->head = new_head;

  return node;
}
