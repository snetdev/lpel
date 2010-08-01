/**
 * Init-queue implementation using a variant of the two-lock queue of
 * Maged M. Michael and Michael L. Scott.
 *
 * Actually, only the Tail-lock is required, as only the owning worker
 * is dequeueing tasks from the head.
 * 
 * Head always points to a dummy node;
 *
 * Pseudocode can be found at:
 * http://www.cs.rochester.edu/research/synchronization/pseudocode/queues.html#tlq
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "initqueue.h"

typedef struct iqnode iqnode_t;

struct initqueue {
  iqnode_t *head;
  iqnode_t *tail;
  pthread_mutex_t lock_tail;
};


struct iqnode {
  iqnode_t *next;
  task_t *task;
};

/**
 * Initialise an init-queue
 */
initqueue_t *InitqueueCreate(void)
{
  initqueue_t *iq = (initqueue_t *) malloc( sizeof(initqueue_t) );

  /* create a first dummy node */
  iqnode_t *node = (iqnode_t *) malloc( sizeof(iqnode_t) );
  /* make it only node in the linked list */
  node->next = NULL;
  node->task = NULL;

  /* both head and tail point to it */
  iq->head = node;
  iq->tail = node;

  pthread_mutex_init(&iq->lock_tail, NULL);

  return iq;
}

/**
 * Cleanup the initqueue
 */
void InitqueueDestroy(initqueue_t *iq)
{
  assert( iq->head->next == NULL ); /* queue empty */

  /* free dummy */
  free(iq->head);
  /* destroy lock */
  pthread_mutex_destroy(&iq->lock_tail);

  free(iq);
}

/**
 * Enqueue a task at the tail
 * - enqueuers synchronize with the lock
 */
void InitqueueEnqueue(initqueue_t *iq, task_t *t)
{
  iqnode_t *node;

  assert( t->next == NULL && t->prev == NULL );

  /* allocate new node */
  node = (iqnode_t *) malloc( sizeof(iqnode_t) );
  node->next = NULL;
  node->task = t;

  /* aquire tail lock */
  while ( 0 != pthread_mutex_trylock(&iq->lock_tail)) {};
  /* link node at the end of the linked list */
  iq->tail->next = node;
  /* swing tail to node */
  iq->tail = node;
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
  task_t *t;
  
  /* with two locks: lock head lock */

  /* read head (dummy) */
  iqnode_t *node = iq->head;
  /* read next pointer */
  iqnode_t *new_head = node->next;

  /* is queue empty? */
  if (new_head == NULL) return NULL;
  
  /* queue is not empty, copy values */
  t = new_head->task;

  /* swing head to next node (becomes new dummy) */
  iq->head = new_head;

  /* with two locks: unlock head lock */

  /* free the node */
  free(node);

  return t;
}
