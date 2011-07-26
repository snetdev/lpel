
#include <stdlib.h>
#include <pthread.h>

//FIXME in build system?
#define MAILBOX_SEMAPHORE

#ifdef MAILBOX_SEMAPHORE
#include <semaphore.h>
#endif /* MAILBOX_SEMAPHORE */

#include <assert.h>

#include "arch/atomic.h"
#include "mailbox.h"


//#define MAILBOX_USE_SPINLOCK

typedef struct mailbox_node_t {
  struct mailbox_node_t *volatile next;
  workermsg_t msg;
  char padding0[64];
} mailbox_node_t;


struct mailbox_t {
  /* free node stack */
  struct {
    mailbox_node_t  *volatile top;
    unsigned long    volatile out_cnt;
  } stack_free __attribute__((aligned(16)));
  char padding0[48];
  /* head */
  mailbox_node_t  *in_head;
  char padding1[56];
  /* tail */
  struct {
    mailbox_node_t     *node;
#ifdef MAILBOX_USE_SPINLOCK
    pthread_spinlock_t  lock;
#else
    pthread_mutex_t     lock;
#endif
  } tail;
  char padding2[64];
#ifdef MAILBOX_SEMAPHORE
  /* counting semaphore */
  sem_t            counter;
  char padding3[64-sizeof(sem_t)];
#endif /* MAILBOX_SEMAPHORE */
};


/******************************************************************************/
/* Free node pool management functions                                        */
/******************************************************************************/

static mailbox_node_t *GetFree( mailbox_t *mbox)
{
  mailbox_node_t * volatile top;
  volatile unsigned long ocnt;
  do {
    ocnt = mbox->stack_free.out_cnt;
    top = mbox->stack_free.top;
    if (!top) return NULL;
  } while( !CAS2( (void**) &mbox->stack_free.top, top, ocnt, top->next, ocnt+1));
  //} while( !compare_and_swap( (void**) &mbox->stack_free.top, top, top->next));

  top->next = NULL;
  return top;
}

static mailbox_node_t *AllocateNode( void)
{
  /* allocate new node */
  mailbox_node_t *n = (mailbox_node_t *)malloc( sizeof( mailbox_node_t));
  n->next = NULL;
  return n;
}

static void PutFree( mailbox_t *mbox, mailbox_node_t *node)
{
  mailbox_node_t * volatile top;
  do {
    top = mbox->stack_free.top;
    node->next = top;
  } while( !compare_and_swap( (void**) &mbox->stack_free.top, top, node));
}



/******************************************************************************/
/* Public functions                                                           */
/******************************************************************************/


mailbox_t *LPEL_EXPORT(MailboxCreate)(void)
{
  mailbox_node_t *n;
  mailbox_t *mbox = (mailbox_t *)malloc(sizeof(mailbox_t));
  int i;

#ifdef MAILBOX_USE_SPINLOCK
  (void) pthread_spin_init( &mbox->tail.lock, PTHREAD_PROCESS_PRIVATE);
#else
  (void) pthread_mutex_init( &mbox->tail.lock, NULL);
#endif

#ifdef MAILBOX_SEMAPHORE
  (void) sem_init( &mbox->counter, 0, 0);
#endif /* MAILBOX_SEMAPHORE */

  mbox->stack_free.top     = NULL;
  mbox->stack_free.out_cnt = 0;

  /* pre-create free nodes */
  for (i=0; i<100; i++) {
    n = (mailbox_node_t *)malloc( sizeof( mailbox_node_t));
    n->next = mbox->stack_free.top;
    mbox->stack_free.top = n;
  }

  /* dummy node */
  n = AllocateNode();

  mbox->in_head = n;
  mbox->tail.node = n;

  return mbox;
}



void LPEL_EXPORT(MailboxDestroy)( mailbox_t *mbox)
{
  mailbox_node_t * volatile top;

  while( LPEL_EXPORT(MailboxHasIncoming)( mbox)) {
    workermsg_t msg;
    LPEL_EXPORT(MailboxRecv)( mbox, &msg);
  }
  /* inbox  empty */
  assert( mbox->in_head->next == NULL );

  /* free dummy */
  PutFree( mbox, mbox->in_head);

  /* free list_free */
  do {
    do {
      top = mbox->stack_free.top;
      if (!top) break;
    } while( !compare_and_swap( (void**) &mbox->stack_free.top, top, top->next));

    if (top) free(top);
  } while(top);

  /* destroy sync primitives */
#ifdef MAILBOX_USE_SPINLOCK
  (void) pthread_spin_destroy( &mbox->tail.lock);
#else
  (void) pthread_mutex_destroy( &mbox->tail.lock);
#endif

#ifdef MAILBOX_SEMAPHORE
  (void) sem_destroy( &mbox->counter);
#endif /* MAILBOX_SEMAPHORE */

  free(mbox);
}



void LPEL_EXPORT(MailboxSend)( mailbox_t *mbox, workermsg_t *msg)
{
  /* get a free node from recepient */
  mailbox_node_t *node = GetFree( mbox);
  if (node == NULL) {
    node = AllocateNode();
  }
  assert( node != NULL);

  /* copy the message */
  node->msg = *msg;

  /* aquire tail lock */
#ifdef MAILBOX_USE_SPINLOCK
  pthread_spin_lock( &mbox->tail.lock);
#else
  pthread_mutex_lock( &mbox->tail.lock);
#endif
  /* link node at the end of the linked list */
  mbox->tail.node->next = node;
  /* swing tail to node */
  mbox->tail.node = node;
  /* release tail lock */
#ifdef MAILBOX_USE_SPINLOCK
  pthread_spin_unlock( &mbox->tail.lock);
#else
  pthread_mutex_unlock( &mbox->tail.lock);
#endif

#ifdef MAILBOX_SEMAPHORE
  /* signal semaphore */
  (void) sem_post( &mbox->counter);
#endif /* MAILBOX_SEMAPHORE */
}


void LPEL_EXPORT(MailboxRecv)( mailbox_t *mbox, workermsg_t *msg)
{
#ifndef MAILBOX_SEMAPHORE
  mailbox_node_t *volatile node, *volatile new_head;
#else /* MAILBOX_SEMAPHORE */
  mailbox_node_t *node, *new_head;
#endif /* MAILBOX_SEMAPHORE */

#ifndef MAILBOX_SEMAPHORE
  do {
#else /* MAILBOX_SEMAPHORE */
  /* wait semaphore */
  (void) sem_wait( &mbox->counter);
#endif /* MAILBOX_SEMAPHORE */

    /* read head (dummy) */
    node = mbox->in_head;
    /* read next pointer */
    new_head = node->next;

#ifndef MAILBOX_SEMAPHORE
    /*TODO burn some cycles */
  } while (new_head == NULL);
#endif /* MAILBOX_SEMAPHORE */

  /* is queue empty? */
  //if (new_head == NULL) return NULL;
  assert( new_head != NULL);

  /* queue is not empty, copy message */
  *msg = new_head->msg;

  /* swing head to next node (becomes new dummy) */
  mbox->in_head = new_head;

  /* put node into free pool */
  PutFree( mbox, node);
}

/**
 * @return 1 if there is an incoming msg, 0 otherwise
 * @note: does not need to be locked as a 'missed' msg
 *        will be eventually fetched in the next worker loop
 */
int LPEL_EXPORT(MailboxHasIncoming)( mailbox_t *mbox)
{
  return ( mbox->in_head->next != NULL );
}
