
#include <malloc.h>
#include <pthread.h>

#include <stdio.h>  /* for perror */
#include <stdlib.h> /* for exit */


#include "streams.h"



/**
 * Create a stream
 */
stream_t *StreamCreate(void)
{
  stream_t *s = (stream_t *) malloc( sizeof(stream_t) );

  s->start = 0;
  s->end = 0;

  s->count = 0;

  /* initialize lock */
  pthread_mutexattr_init( &(s->lockattr) );
  pthread_mutexattr_settype( &(s->lockattr), PTHREAD_MUTEX_ERRORCHECK_NP );
  pthread_mutex_init( &(s->lock), &(s->lockattr) );

  s->notempty = false;
  s->notfull = false;

  s->set = NULL;

  s->next = NULL;

  return s;
}


/**
 * Destroy a stream
 */
void StreamDestroy(stream_t *s)
{
  /* release resources of lock */
  pthread_mutex_destroy( &(s->lock) );
  pthread_mutexattr_destroy( &(s->lockattr) );

  /* free memory */
  free(s);
}


/*
 * Blocking read
 */
item_t StreamRead(stream_t *s)
{
  item_t item;

  LOCK(s->lock);

  while( s->count == 0 ) {
    /* CWAIT(s->notfull):
     *   s->notempty = false;
     *   TASK->waiting_on = &(s->notempty);
     *   UNLOCK(s->lock);
     *   yield(); // co_resume()
     *   LOCK(s->lock);
     */
  }

  /* buffer is not empty */
  item = s->buffer[s->start];
  s->start = s->start + 1;
  if (s->start == BUFFER_SIZE) {
    s->start = 0;
  }
  
  s->count--;
  /* CNOTIFY(s->notfull):
   *   s->notfull = true;
   */
  if (s->set != NULL) {
    /* enter streamset */
    LOCK(s->set->lock);
    s->set->count_input--;
    if (s->count == 0) {
      s->set->count_new--;
    }
    /* exit streamset */
    UNLOCK(s->set->lock);
  }

  UNLOCK(s->lock);
  return item;
}


/*
 * Unblocking read
 */
item_t StreamPeek(stream_t *s)
{
  item_t item = NULL;

  LOCK(s->lock);
  if (s->count != 0) {
    item = s->buffer[s->start];
  }
  UNLOCK(s->lock);

  return item;
}


/*
 * Blocking write
 */
void StreamWrite(stream_t *s, item_t item)
{
  LOCK(s->lock);

  while( s->count == BUFFER_SIZE ) {
    /* CWAIT(s->notfull) */
  }
  
  /* there is space left */
  s->buffer[s->end] = item;
  s->end = (s->end + 1);
  if (s->end == BUFFER_SIZE) {
    s->end = 0;
  }
  
  s->count++;
  /* CNOTIFY(s->notempty) */
 

  if (s->set != NULL) {
    /* enter streamset */
    LOCK(s->set->lock)
    s->set->count_input++;
    if (s->count == 1) {
      s->set->count_new++;
    }
    /* CNOTIFY(s->set->new_input) */
    /* exit streamset */
    UNLOCK(s->set->lock);
  }

  UNLOCK(s->lock);
}


/**
 * return the number of items in the stream
 */
unsigned int StreamCount(stream_t *s)
{
  return s->count;
}

