
#include <malloc.h>
#include <pthread.h>

#include <stdio.h>  /* for perror */
#include <stdlib.h> /* for exit */

#include "bool.h"
#include "streams.h"


/*
**  Restrictions for a streamset:
**
**  - a streamset must not be shared between multiple tasks, i.e.,
**    a streamset is held by one consumer
**
**  - a stream must only be part of one streamset
*/



/**
 * Create s streamset
 */
streamset_t *StreamsetCreate(void)
{
  streamset_t *set = (streamset_t *) malloc( sizeof(streamset_t) );
  
  set->hook = NULL;
  set->count_streams = 0;
  set->count_input = 0;
  set->count_new = 0;

  /* init lock */
  pthread_mutexattr_init( &(set->lockattr) );
  pthread_mutexattr_settype( &(set->lockattr), PTHREAD_MUTEX_ERRORCHECK_NP ); 
  pthread_mutex_init( &(set->lock), &(set->lockattr) );

  return set;
}



/**
 * Destroy a streamset
 */
void StreamsetDestroy(streamset_t *set)
{
  /* set must be empty! */
  /* assert set->hook == NULL */
  
  /* destroy lock */
  pthread_mutex_destroy( &(set->lock));
  pthread_mutexattr_destroy( &(set->lockattr) );

  free(set);
  set = NULL;
}


/**
 * Add a stream to a streamset
 */
void StreamsetAdd(streamset_t *set, stream_t *s)
{
  LOCK(s->lock);
  
  /* check if stream is not part of another set */
  /* assert s->set == NULL */

  if (set->hook == NULL) {

    /* set is empty */
    set->hook = s;
    s->next = s; /* selfloop */
    s->set = set;

  } else {
    
    /* insert stream between last element and first element=hook */
    stream_t *sptr = set->hook;
    while (sptr->next != set->hook)
      sptr = sptr->next;

    /* sptr points to the last stream: insert */
    sptr->next = s;
    s->next = set->hook;
    s->set = set;
  }

  LOCK(set->lock);
  set->count_streams++;
  if (s->count > 0) {
    set->count_input += s->count;
    set->count_new++;
  }
  UNLOCK(set->lock);

  UNLOCK(s->lock);
}



/**
 * Remove a stream from a streamset
 */
void StreamsetRemove(streamset_t *set, stream_t *s)
{
  LOCK(s->lock);

  /* check if stream is part of this streamset */
  /* assert s->set == set */

  if (s->next == s) {
    
    /* selfloop: is single element in set */
    set->hook = NULL;
    s->next = NULL;
    s->set = NULL;

  } else {
  
    /* find element that points to s */
    stream_t *sptr = set->hook;
    while (sptr->next != s) {
      sptr = sptr->next;
    }

    /* if hook points to s, let hook point to successor */
    if (set->hook == s) {
      set->hook = s->next;
    }
    
    sptr->next = s->next;
    s->next = NULL;
    s->set = NULL;
  }

  LOCK(set->lock);
  set->count_streams--;
  if (s->count > 0) {
    set->count_input -= s->count;
    set->count_new--;
  }
  UNLOCK(set->lock);

  UNLOCK(s->lock);
}


/**
 * Get the total number of elements in all streams
 * - if average == true, return the average
 *   (if there is at least one element in any stream, it returns at least 1)
 */
unsigned int StreamsetCountInput(streamset_t *set, bool average)
{
  unsigned int cnt;

  if (average) {
    /* lock for consistency of count_streams and count_input */
    LOCK(set->lock);
    cnt = set->count_input / set->count_streams;
    if ( (cnt==0) && (set->count_input>0) ) {
      cnt = 1;
    }
    UNLOCK(set->lock);
  } else {
    cnt = set->count_input;
  }
  return cnt;
}


/**
 *  find the first stream of the set which contains an item
 *  - if there is a nonempty stream, it is stored in s
 *  - otherwise, s remains untouched
 */
static bool StreamsetPeek(streamset_t *set, stream_t **s)
{
  stream_t *sptr = set->hook;
  do {
    void *item = StreamPeek(sptr);
    if (item != NULL) {
      *s = sptr;
      return true;
    }
    sptr = sptr->next;
  } while (sptr != set->hook);
  return false;
}

/**
 * Blocks while no new item is the first item of any stream
 * - if one stream is non-empty, it returns the stream together
 *   with its peek result
 */
struct pair StreamsetPeekMany(streamset_t *set)
{
  stream_t *s = NULL;
  struct pair stream_item;
  bool peek_success;

  LOCK(set->lock);
  while( set->count_new == 0 ) {
    /* CWAIT(set->notempty)*/
  }
  UNLOCK(set->lock);
  
  peek_success = StreamsetPeek(set, &s);
  /* at this point, a non-empty stream will be found,
     as there is no other consumer */
  /* assert peek_success */

  stream_item.s = s;
  stream_item.item = StreamPeek(s);

  set->hook = s;
  /* optionally rotate streamset */
  /* set->hook = s->next; */

  return stream_item;
}

