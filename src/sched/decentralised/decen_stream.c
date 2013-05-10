#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#include <lpel.h>

#include "arch/atomic.h"
#include "lpelcfg.h"
#include "buffer.h"
#include "task.h"

#include "stream.h"
#include "lpel/monitor.h"


static atomic_int stream_seq = ATOMIC_VAR_INIT(0);

/**
 * Create a stream
 *
 * Allocate and initialize memory for a stream.
 *
 * @return pointer to the created stream
 */
lpel_stream_t *LpelStreamCreate(int size)
{
  assert( size >= 0);
  if (0==size) size = STREAM_BUFFER_SIZE;

  /* allocate memory for both the stream struct and the buffer area */
  lpel_stream_t *s = (lpel_stream_t *) malloc( sizeof(lpel_stream_t) );

  /* reset buffer (including buffer area) */
  s->buffer = LpelBufferInit( size);

  s->uid = atomic_fetch_add( &stream_seq, 1);
  PRODLOCK_INIT( &s->prod_lock );
  atomic_init( &s->n_sem, 0);
  atomic_init( &s->e_sem, size);
  s->is_poll = 0;
  s->prod_sd = NULL;
  s->cons_sd = NULL;
  s->usr_data = NULL;
  s->sched_info = NULL;
  return s;
}



/**
 * Blocking write to a stream
 *
 * If the stream is full, the task is suspended until the consumer
 * reads items from the stream, freeing space for more items.
 *
 * @param sd    stream descriptor
 * @param item  data item (a pointer) to write
 * @pre         current task is single writer
 * @pre         item != NULL
 */
void LpelStreamWrite( lpel_stream_desc_t *sd, void *item)
{
  lpel_task_t *self = sd->task;
  int poll_wakeup = 0;

  /* check if opened for writing */
  assert( sd->mode == 'w' );
  assert( item != NULL );

  /* MONITORING CALLBACK */
#ifdef USE_TASK_EVENT_LOGGING
  if (sd->mon && MON_CB(stream_writeprepare)) {
    MON_CB(stream_writeprepare)(sd->mon, item);
  }
#endif

  /* quasi P(e_sem) */
  if ( atomic_fetch_sub( &sd->stream->e_sem, 1)== 0) {

	/* MONITORING CALLBACK */
#ifdef USE_TASK_EVENT_LOGGING
    if (sd->mon && MON_CB(stream_blockon)) {
      MON_CB(stream_blockon)(sd->mon);
    }
#endif

    /* wait on stream: */
    LpelTaskBlockStream( self);
  }

  /* writing to the buffer and checking if consumer polls must be atomic */
  PRODLOCK_LOCK( &sd->stream->prod_lock);
  {
    /* there must be space now in buffer */
    assert( LpelBufferIsSpace( sd->stream->buffer) );
    /* put item into buffer */
    LpelBufferPut( sd->stream->buffer, item);

    if ( sd->stream->is_poll) {
      /* get consumer's poll token */
      poll_wakeup = atomic_exchange( &sd->stream->cons_sd->task->poll_token, 0);
      sd->stream->is_poll = 0;
    }
  }
  PRODLOCK_UNLOCK( &sd->stream->prod_lock);



  /* quasi V(n_sem) */
  if ( atomic_fetch_add( &sd->stream->n_sem, 1) < 0) {
    /* n_sem was -1 */
    lpel_task_t *cons = sd->stream->cons_sd->task;
    /* wakeup consumer: make ready */
    LpelTaskUnblock( self, cons);

    /* MONITORING CALLBACK */
#ifdef USE_TASK_EVENT_LOGGING
    if (sd->mon && MON_CB(stream_wakeup)) {
      MON_CB(stream_wakeup)(sd->mon);
    }
#endif
  } else {
    /* we are the sole producer task waking the polling consumer up */
    if (poll_wakeup) {
      lpel_task_t *cons = sd->stream->cons_sd->task;
      cons->wakeup_sd = sd->stream->cons_sd;

      LpelTaskUnblock( self, cons);

      /* MONITORING CALLBACK */
#ifdef USE_TASK_EVENT_LOGGING
      if (sd->mon && MON_CB(stream_wakeup)) {
        MON_CB(stream_wakeup)(sd->mon);
      }
#endif
    }
  }

  /* MONITORING CALLBACK */
#ifdef USE_TASK_EVENT_LOGGING
  if (sd->mon && MON_CB(stream_writefinish)) {
    MON_CB(stream_writefinish)(sd->mon);
  }
#endif

}




/**
 * Blocking, consuming read from a stream
 *
 * If the stream is empty, the task is suspended until
 * a producer writes an item to the stream.
 *
 * @param sd  stream descriptor
 * @return    the next item of the stream
 * @pre       current task is single reader
 */
void *LpelStreamRead( lpel_stream_desc_t *sd)
{
  void *item;
  lpel_task_t *self = sd->task;

  assert( sd->mode == 'r');

  /* MONITORING CALLBACK */
#ifdef USE_TASK_EVENT_LOGGING
  if (sd->mon && MON_CB(stream_readprepare)) {
    MON_CB(stream_readprepare)(sd->mon);
  }
#endif

  /* quasi P(n_sem) */
  if ( atomic_fetch_sub( &sd->stream->n_sem, 1) == 0) {

#ifdef USE_TASK_EVENT_LOGGING
    /* MONITORING CALLBACK */
    if (sd->mon && MON_CB(stream_blockon)) {
      MON_CB(stream_blockon)(sd->mon);
    }
#endif

    /* wait on stream: */
    LpelTaskBlockStream( self);
  }


  /* read the top element */
  item = LpelBufferTop( sd->stream->buffer);
  assert( item != NULL);
  /* pop off the top element */
  LpelBufferPop( sd->stream->buffer);


  /* quasi V(e_sem) */
  if ( atomic_fetch_add( &sd->stream->e_sem, 1) < 0) {
    /* e_sem was -1 */
    lpel_task_t *prod = sd->stream->prod_sd->task;
    /* wakeup producer: make ready */
    LpelTaskUnblock( self, prod);

    /* MONITORING CALLBACK */
#ifdef USE_TASK_EVENT_LOGGING
    if (sd->mon && MON_CB(stream_wakeup)) {
      MON_CB(stream_wakeup)(sd->mon);
    }
#endif

  }

  /* MONITORING CALLBACK */
#ifdef USE_TASK_EVENT_LOGGING
  if (sd->mon && MON_CB(stream_readfinish)) {
    MON_CB(stream_readfinish)(sd->mon, item);
  }
#endif
  return item;
}

/**
 * Close a stream previously opened for reading/writing
 *
 * @param sd          stream descriptor
 * @param destroy_s   if != 0, destroy the stream as well
 */
void LpelStreamClose( lpel_stream_desc_t *sd, int destroy_s)
{
  /* MONITORING CALLBACK */
#ifdef USE_TASK_EVENT_LOGGING
  if (sd->mon && MON_CB(stream_close)) {
    MON_CB(stream_close)(sd->mon);
  }
#endif

  if (destroy_s) {
    LpelStreamDestroy( sd->stream);
  }
  free(sd);
}



/**
 * Replace a stream opened for reading by another stream
 * Destroys old stream.
 *
 * @param sd    stream descriptor for which the stream must be replaced
 * @param snew  the new stream
 * @pre         snew must not be opened by same or other task
 */
void LpelStreamReplace( lpel_stream_desc_t *sd, lpel_stream_t *snew)
{
  assert( sd->mode == 'r');

  /* destroy old stream */
  LpelStreamDestroy( sd->stream);
  /* assign new stream */
  sd->stream = snew;
  /* new consumer sd of stream */
  sd->stream->cons_sd = sd;


  /* MONITORING CALLBACK */
#ifdef USE_TASK_EVENT_LOGGING
  if (sd->mon && MON_CB(stream_replace)) {
    MON_CB(stream_replace)(sd->mon, snew->uid);
  }
#endif

}

/**
  * Open a stream for reading/writing
 *
 * @param s     pointer to stream
 * @param mode  either 'r' for reading or 'w' for writing
 * @return      a stream descriptor
 * @pre         only one task may open it for reading resp. writing
 *              at any given point in time
 */
lpel_stream_desc_t *LpelStreamOpen( lpel_stream_t *s, char mode)
{
  lpel_stream_desc_t *sd;
  lpel_task_t *ct = LpelTaskSelf();

  assert( mode == 'r' || mode == 'w' );
  sd = (lpel_stream_desc_t *) malloc( sizeof( lpel_stream_desc_t));
  sd->task = ct;
  sd->stream = s;
  sd->mode = mode;
  sd->next  = NULL;

#ifdef USE_TASK_EVENT_LOGGING
  /* create monitoring object, or NULL if stream
   * is not going to be monitored (depends on ct->mon)
   */
  if (ct->mon && MON_CB(stream_open)) {
    sd->mon = MON_CB(stream_open)( ct->mon, s->uid, mode);
  } else {
    sd->mon = NULL;
  }
#else
  sd->mon = NULL;
#endif

  switch(mode) {
    case 'r': s->cons_sd = sd; break;
    case 'w': s->prod_sd = sd; break;
  }

  return sd;
}

/**
 * Destroy a stream
 *
 * Free the memory allocated for a stream.
 *
 * @param s   stream to be freed
 * @pre       stream must not be opened by any task!
 */
void LpelStreamDestroy( lpel_stream_t *s)
{
  PRODLOCK_DESTROY( &s->prod_lock);
  atomic_destroy( &s->n_sem);
  atomic_destroy( &s->e_sem);
  LpelBufferCleanup( s->buffer);
  free( s);
}
