
#include <unistd.h>
#include <malloc.h>
#include <pthread.h>
#include <pcl.h>
#include "bool.h"

#include "scheduler.h"

#ifndef BUFFER_SIZE
#define BUFFER_SIZE 10+1
#endif



/*
 * if the real worker threads are not preempted,
 * one might consider using spinlocks instead of mutexes.
 *
 * if order and atomicity can be guaranteed, a lock-free version
 * could be employed.
 */


#define STREAM_LOCK(s) \
  if (pthread_mutex_lock( &(s->lock) ) != 0) { \
    perror("stream lock failed"); \
    exit(-1); \
  }

#define STREAM_UNLOCK(s) \
  if (pthread_mutex_unlock( &(s->lock) ) != 0) { \
    perror("stream unlock failed"); \
    exit(-1); \
  }

/*
 * A Stream
 */
typedef struct {
  /* fields for circular buffer storage */
  void *buffer[BUFFER_SIZE];
  unsigned int start; /* position from where to consume */
  unsigned int end;   /* position where to insert new items */

  /* tasks to suspend/notify */
  task_t *producer;
  task_t *consumer;

  /* mutual exclusion for a single worker thread */
  pthread_mutex_t lock;
} stream_t;



/*
 * Check if stream is empty
 */
static bool StreamIsEmpty(stream_t *s)
{
  bool isempty;
  
  STREAM_LOCK(s);
  isempty = s->start == s->end;
  STREAM_UNLOCK(s);

  return isempty;
}

/*
 * Check if stream is full
 * - in this implementation, always one slot is kept empty
 */
static bool StreamIsFull(stream_t *s)
{
  bool isfull;

  STREAM_LOCK(s);
  isfull = ((s->end + 1) % BUFFER_SIZE) == s->start;
  STREAM_UNLOCK(s);

  return isfull;
}


/*
 * Create a stream
 */
stream_t *StreamCreate(task_t *producer)
{
  stream_t *s = (stream_t *) malloc( sizeof(stream_t) );
  s->start = 0;
  s->end = 0;
  
  s->producer = producer;
  s->consumer = NULL;
  
  s->lock = PTHREAD_MUTEX_INITIALIZER;

}



/*
 * Delete a stream
 */
void StreamDelete(stream_t *s)
{
  /* release resources of mutex */
  pthread_mutex_destroy( s->lock );

  /* free memory */
  free(s);
}


void StreamSetConsumer(stream_t *s, task_t *consumer)
{
  s->consumer = consumer;

  if (!StreamIsEmpty(s)) {
    /* Signal consumer (if not NULL)
     * LOCK?
     * TODO
     */
  }
}


/*
 * Put an item into the stream
 */
void StreamPut(stream_t *s, void *item)
{
  /* buffer full, block producer */
  if ( StreamIsFull(s) ) {
    /* put producer in a blocked state
     * i.e. do not put it on a ready queue and
     * return to the scheduler
     */
    co_resume();
  }
 
  STREAM_LOCK(s);
  s->buffer[ s->end ] = item;
  s->end = (s->end + 1) % BUFFER_SIZE;
  STREAM_UNLOCK(s);

  /* signal the consumer (if not NULL -> LOCK!)
   * - if not on a ready queue, put on a ready queue (or a reschedule queue)
   *   (state==blocked)
   * TODO
   */
}


/*
 * Get an item from the stream (blocking)
 */
void *StreamGet(stream_t *s)
{
  void *item;
  
  /* buffer empty, block consumer */
  if ( StreamIsEmpty(s) ) {
    /* put consumer in a blocked state
     * i.e. do not put it on a ready queue and
     * return to the scheduler
     */
    co_resume();
  }

  /* optionally: maintain a flag in task that shows if a task has already
   * consumed a value. if it has, the task might be rescheduled (in a ready
   * queue). as there is only a single consumer, execution might be resumed
   * at this point later on, i.e., do a co_resume() here.
   * TODO
   */
  
  STREAM_LOCK(s);
  item = s->buffer[ s->start ];
  s->start = (s->start + 1) % BUFFER_SIZE;
  STREAM_UNLOCK(s);

  /* signal the producer (always != NULL)
   * - if not on a ready queue, put on a ready queue (or a reschedule queue)
   *   (state==blocked)
   * TODO
   */

  return item;
}


/*
 * Get an item from the stream (non-blocking)
 */
void *StreamTryGet(stream_t *s)
{
  void *item = NULL;
  bool hasitem = false;

  STREAM_LOCK(s);
  if (s->start != s->end) {
    item = s->buffer[ s->start ];
    s->start = (s->start + 1) % BUFFER_SIZE;
    hasitem = true;
  }
  STREAM_UNLOCK(s);

  if (hasitem) {
    /* signal the producer */
    //TODO
  }
  return item;
}

/*
 * Peek an item
 * - the item is not removed
 * - if stream is empty, NULL is returned
 */
void *StreamPeek(stream_t *s)
{
  void *item = NULL;

  STREAM_LOCK(s);
  if (s->start != s->end) {
    item = s->buffer[ s->start ];
  }
  STREAM_UNLOCK(s);

  return item;
}

unsigned int StreamSize(stream_t *s)
{
  /* no need for locking here, as size is never changed */
  return BUFFER_SIZE - 1;
}

unsigned int StreamNumItems(stream_t *s)
{
  unsigned int num;

  STREAM_LOCK(s);
  num = (BUFFER_SIZE + s->end - s->start) % BUFFER_SIZE;
  STREAM_UNLOCK(s);

  return num;
}

unsigned int StreamSpace(stream_t *s)
{
  unsigned int space;

  STREAM_LOCK(s);
  space = (BUFFER_SIZE + s->start - s->end - 1) % BUFFER_SIZE;
  STREAM_UNLOCK(s);

  return space;
}
