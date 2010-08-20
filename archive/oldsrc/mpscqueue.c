/**
 * Multilpe-Producer Single-Consumer Queue implementation
 * using a variant of the two-lock queue of
 * Maged M. Michael and Michael L. Scott.
 *
 * Actually, only the Tail-lock is required, as only a single consumer
 * is dequeueing items from the head.
 * 
 * Head always points to a dummy node;
 *
 * Pseudocode can be found at:
 * http://www.cs.rochester.edu/research/synchronization/pseudocode/queues.html#tlq
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#include "mpscqueue.h"

typedef struct qnode qnode_t;

struct mpscqueue {
  qnode_t *head;
  qnode_t *tail;
  pthread_mutex_t lock_tail;
};


struct qnode {
  qnode_t *next;
  void *item;
};

/**
 * Initialise the MPSC queue
 */
mpscqueue_t *MPSCQueueCreate(void)
{
  mpscqueue_t *q = (mpscqueue_t *) malloc( sizeof(mpscqueue_t) );

  /* create a first dummy node */
  qnode_t *node = (qnode_t *) malloc( sizeof(qnode_t) );
  /* make it only node in the linked list */
  node->next = NULL;
  node->item = NULL;

  /* both head and tail point to it */
  q->head = node;
  q->tail = node;

  pthread_mutex_init(&q->lock_tail, NULL);

  return q;
}

/**
 * Destroy an MPSC queue
 *
 * @param q   queue which to destroy
 */
void MPSCQueueDestroy(mpscqueue_t *q)
{
  assert( q->head->next == NULL ); /* queue empty */

  /* free dummy */
  free(q->head);
  /* destroy lock */
  pthread_mutex_destroy(&q->lock_tail);

  free(q);
}

/**
 * Enqueue an item at the tail
 * - enqueuers synchronize with the lock
 *
 * @param q       queue in which to enqueue
 * @param item    item to enqueue
 * @pre           item != NULL
 */
void MPSCQueueEnqueue(mpscqueue_t *q, void *item)
{
  qnode_t *node;
  
  assert(item != NULL);

  /* allocate new node */
  node = (qnode_t *) malloc( sizeof(qnode_t) );
  node->next = NULL;
  node->item = item;

  /* aquire tail lock */
  while ( 0 != pthread_mutex_trylock(&q->lock_tail)) /*spin*/;
  /* link node at the end of the linked list */
  q->tail->next = node;
  /* swing tail to node */
  q->tail = node;
  /* release tail lock */
  pthread_mutex_unlock(&q->lock_tail);
}

/**
 * Dequeue an item from the head
 *
 * - supposed only to be called by a single consumer, hence
 *   compared to the original implementation, does not need a head-lock
 * 
 * @param q   queue which to dequeue from
 * @return    item from the head or NULL if queue is empty
 */
void* MPSCQueueDequeue(mpscqueue_t *q)
{
  void *item;
  
  /* with two locks: lock head lock */

  /* read head (dummy) */
  qnode_t *node = q->head;
  /* read next pointer */
  qnode_t *new_head = node->next;

  /* is queue empty? */
  if (new_head == NULL) return NULL;
  
  /* queue is not empty, copy values */
  item = new_head->item;

  /* swing head to next node (becomes new dummy) */
  q->head = new_head;

  /* with two locks: unlock head lock */

  /* free the node */
  free(node);

  return item;
}
