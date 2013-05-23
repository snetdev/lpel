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
#include "hrc_worker.h"
#include "stream.h"
#include "hrc_stream.h"
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
  lpel_stream_t *s = LpelWorkerGetStream();
  if (!s) {
  	s = (lpel_stream_t *) malloc( sizeof(lpel_stream_t) );
  	s->buffer = LpelBufferInit( size);
  	s->sched_info = (stream_sched_info_t *) malloc(sizeof(stream_sched_info_t));
  }

  s->uid = atomic_fetch_add( &stream_seq, 1);
  PRODLOCK_INIT( &s->prod_lock );
  atomic_init( &s->n_sem, 0);
  atomic_init( &s->e_sem, size);
  s->is_poll = 0;
  s->prod_sd = NULL;
  s->cons_sd = NULL;
  s->usr_data = NULL;
  s->sched_info->is_entry = 0;
  s->sched_info->is_exit = 0;
  s->sched_info->write = 0;
  s->sched_info->read = 0;
  s->next = NULL;
  return s;
}



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
  if (sd->stream->sched_info->is_entry) {
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


  sd->stream->sched_info->write++;

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

  sd->stream->sched_info->read++;

  /* only entry streams are bounded */
  if (sd->stream->sched_info->is_entry) {
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

  workerctx_t *wc = sd->task->worker_context;
  if (destroy_s) {
  	assert(sd->mode == 'r');
  	lpel_stream_t *s = sd->stream;
  	assert(LpelBufferIsEmpty(s->buffer));
  	s->prod_sd->stream = NULL;
  	s->prod_sd = NULL;
  	s->cons_sd = NULL;
    LpelWorkerPutStream(wc, sd->stream);
    sd->stream = NULL;
  }
  LpelTaskRemoveStream(sd->task, sd, sd->mode);
  sd->task = NULL;
  LpelWorkerPutSd(wc, sd);
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
  snew->sched_info->is_exit = sd->stream->sched_info->is_exit;
  snew->sched_info->is_entry = sd->stream->sched_info->is_entry; /* technically, no need to set as entry stream shouldn't be replaced
    																				however it is set here for special case in source/sink mode */

  /* free the old stream */
  workerctx_t *wc = sd->task->worker_context;
  lpel_stream_t *s = sd->stream;
  s->prod_sd->stream = NULL;
  s->prod_sd = NULL;
  s->cons_sd = NULL;
  LpelWorkerPutStream(wc, sd->stream);


  /* assign new stream */
  lpel_stream_desc_t *old_cons = snew->cons_sd;
  old_cons->stream = NULL;
  snew->cons_sd = sd;
  sd->stream = snew;


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
  sd = LpelWorkerGetSd(ct->worker_context);
  if (sd == NULL)
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

  /* add stream desc to the task, used for calculate dynamic task priority in HRC
   * This function has no effect in DECEN
   * It is implemented this way to avoid 2 implementations: one for HRC and one for DECEN
   */
  LpelTaskAddStream(ct, sd, mode);

  /* set entry/exit stream */
  if (LpelTaskIsWrapper(ct) && (mode == 'r'))
  	s->sched_info->is_exit = 1;
  if (LpelTaskIsWrapper(ct) && (mode == 'w'))
    	s->sched_info->is_entry = 1;
  return sd;
}


/*
 * return the stream's consumer
 */
lpel_task_t *LpelStreamConsumer(lpel_stream_t *s) {
	if (!s)
		return NULL;
	if (!s->cons_sd)
		return NULL;
	return s->cons_sd->task;
}

/*
 * return the stream's producer
 */
lpel_task_t *LpelStreamProducer(lpel_stream_t *s) {
	if (!s)
		return NULL;
	if (!s->prod_sd)
		return NULL;
	return s->prod_sd->task;
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
  free(s->sched_info);
  free( s);
}

int LpelStreamIsEntry(lpel_stream_t *s) {
	return s->sched_info->is_entry;
}
int LpelStreamIsExit(lpel_stream_t *s) {
	return s->sched_info->is_exit;
}

int LpelStreamFillLevel(lpel_stream_t *s) {
	if (s == NULL)
		return 0;
	return s->sched_info->write - s->sched_info->read;
}
