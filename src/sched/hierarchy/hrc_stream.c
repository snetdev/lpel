#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <hrc_lpel.h>

#include "arch/atomic.h"
#include "lpelcfg.h"
#include "buffer.h"
#include "task.h"
#include "hrc_task.h"

#include "stream.h"
#include "lpel/monitor.h"

/**
 * Blocking write to a stream
 *
 * HRC stream is unbounded, except for entry stream.
 * If entry stream is full, the task is suspended until the consumer
 * reads items from the stream, freeing space for more items.
 *
 * After writing one message record,
 * if the number of written messages (in the current dispatch) reaches the limit,
 * the task yield itself
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


  /* only entry streams are bounded */
  if (sd->stream->is_entry) {
  	/* quasi P(e_sem) */
  	if ( atomic_fetch_sub( &sd->stream->e_sem, 1)== 0) {

  		/* MONITORING CALLBACK */
#ifdef USE_TASK_EVENT_LOGGING
  		if (sd->mon && MON_CB(stream_blockon)) {
  			MON_CB(stream_blockon)(sd->mon);
  		}
#endif

  		/* block source tasks wait on stream: */
  		LpelTaskBlockStream( self);
  	}
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

  /* check if a task should be yield after writing this output record */
  LpelTaskCheckYield(self);
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


  /* only entry streams are bounded */
  if (sd->stream->is_entry) {
  	/* quasi V(e_sem) */
  	if ( atomic_fetch_add( &sd->stream->e_sem, 1) < 0) {
  		/* e_sem was -1 */
  		lpel_task_t *prod = sd->stream->prod_sd->task;
  		/* wakeup source tasks: make ready */
  		LpelTaskUnblock( self, prod);
  	}
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

  /*
   * if stream is not destroyed yet, update prod_sd and cons_sd
   * */
  if (sd->stream) {
  	PRODLOCK_LOCK( &sd->stream->sd_lock);
  	if (sd->mode == 'r' && sd->stream->cons_sd == sd) {		// stream is not replaced yet
  		sd->stream->cons_sd = NULL;

  		if (sd->stream->prod_sd && destroy_s)
  			sd->stream->prod_sd->stream = NULL;		// set this so that the task on the other side of stream does not access to stream after stream is destroy
  	}
  	if (sd->mode == 'w' && sd->stream->prod_sd == sd) {	// stream is not replaced yet
  		sd->stream->prod_sd = NULL;
  		if(sd->stream->cons_sd && destroy_s)
  			sd->stream->cons_sd->stream = NULL; // set this so that the task on the other side of stream does not access to stream after stream is destroy
  	}
  	PRODLOCK_UNLOCK( &sd->stream->sd_lock);
  }

  if (destroy_s) {
		LpelStreamDestroy( sd->stream);
	}

  LpelTaskRemoveStream(sd->task, sd, sd->mode);
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
  /* set exit/entry stream if needed */
  snew->is_exit = sd->stream->is_exit;
  snew->is_entry = sd->stream->is_entry; /* technically, no need to set as entry stream shouldn't be replaced
    																				however it is set here for special case in source/sink mode */

  /* destroy old stream */
  LpelStreamDestroy( sd->stream);
  /* assign new stream */
  sd->stream = snew;

  /* new consumer sd of stream */
  PRODLOCK_LOCK( &sd->stream->sd_lock);
  sd->stream->cons_sd = sd;
  PRODLOCK_UNLOCK( &sd->stream->sd_lock);

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

  /*
   * the first time set cons_sd/prod_sd, no need to use lock
   */
  switch(mode) {
    case 'r': s->cons_sd = sd; break;
    case 'w': s->prod_sd = sd; break;
  }

  /* add stream desc to the task, used for calculate dynamic task priority in HRC
   * This function has no effect in DECEN
   * It is implemented this way to avoid 2 implementations: one for HRC and one for DECEN
   */
  LpelTaskAddStream(ct, sd, mode);

  /* set entry/exit stream */
  if (LpelTaskIsWrapper(ct) && (mode == 'r'))
  	s->is_exit = 1;
  if (LpelTaskIsWrapper(ct) && (mode == 'w'))
    	s->is_entry = 1;

  return sd;
}


/*
 * return the stream's consumer
 */
lpel_task_t *LpelStreamConsumer(lpel_stream_t *s) {
	if (!s)
		return NULL;
	lpel_task_t *t;
	PRODLOCK_LOCK( &s->sd_lock);
	if (s->cons_sd != NULL)
		t = s->cons_sd->task;
	else
		t = NULL;
	PRODLOCK_UNLOCK( &s->sd_lock);
	return t;
}

/*
 * return the stream's producer
 */
lpel_task_t *LpelStreamProducer(lpel_stream_t *s) {
	if (!s)
		return NULL;

	lpel_task_t *t;
	PRODLOCK_LOCK( &s->sd_lock);
	if (s->prod_sd != NULL)
		t = s->prod_sd->task;
	else
		t = NULL;
	PRODLOCK_UNLOCK( &s->sd_lock);
	return t;
}

