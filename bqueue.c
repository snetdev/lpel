
#include <assert.h>

#include "bqueue.h"

void BQueueInit( bqueue_t *bq)
{
  TaskqueueInit( &bq->queue);
  pthread_mutex_init( &bq->lock, NULL);
  pthread_cond_init( &bq->cond, NULL);
  bq->terminate = false;
}

void BQueueCleanup( bqueue_t *bq)
{
  assert( bq->terminate == true );
  pthread_mutex_destroy( &bq->lock);
  pthread_cond_destroy( &bq->cond);
}


void BQueuePut( bqueue_t *bq, task_t *t)
{
  pthread_mutex_lock( &bq->lock );
  TaskqueuePushBack( &bq->queue, t );
  if ( 1 == bq->queue.count ) {
    pthread_cond_signal( &bq->cond );
  }
  pthread_mutex_unlock( &bq->lock );
}

bool BQueueFetch( bqueue_t *bq, task_t **t)
{
  bool terminate;
  pthread_mutex_lock( &bq->lock);
  terminate = bq->terminate;
  while( 0 == bq->queue.count && !terminate) {
    pthread_cond_wait( &bq->cond, &bq->lock);
    terminate = bq->terminate;
  }
  *t = TaskqueuePopFront( &bq->queue);
  pthread_mutex_unlock( &bq->lock);
  return terminate;
}

void BQueueTerm( bqueue_t *bq)
{
  pthread_mutex_lock( &bq->lock );
  bq->terminate = true;
  pthread_cond_signal( &bq->cond );
  pthread_mutex_unlock( &bq->lock );
}



