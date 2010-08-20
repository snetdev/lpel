
#include <stdlib.h>
#include <assert.h>

#include "mpsctq.h"

#include "sysdep.h"


#define SWAP xchg


void MpscTqInit(mpsctq_t *q)
{
  q->head = &q->stub;
  q->tail = &q->stub;
  q->stub.next = NULL;
}

void MpscTqCleanup(mpsctq_t *q)
{ /*NOP*/ }


void MpscTqEnqueue(mpsctq_t *q, task_t *t)
{
  task_t *prev;
  
  assert( t->next == NULL );
  prev = SWAP( (int *) &q->head, (int) t);
  /* (point where list is disconnected) */
  prev->next = t;
}

task_t *MpscTqDequeue(mpsctq_t *q)
{
  task_t *tail, *next;

  tail = q->tail;
  next = tail->next;
  
  /* skip stub, if it is not the only element in list */
  if ( tail == &q->stub && next != NULL ) {
    q->tail = next;
    tail = next;
    next = next->next;
    /* unlink skipped stub */
    q->stub.next = NULL;
  }

  /* tail and head point to single 'useful' element: insert stub */
  if ( tail != &q->stub && tail == q->head ) {
    assert( q->stub.next == NULL );
    MpscTqEnqueue( q, &q->stub );
    next = tail->next;
  }

  /* return and advance tail */
  if ( next != NULL ) {
    q->tail = next;
    /* unlink the task which is returned */
    tail->next = NULL;
    return tail;
  }
  
  /* otherwise */
  return NULL; 
}

