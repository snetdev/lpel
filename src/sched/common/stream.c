/**
 * File: stream.c
 * Auth: Daniel Prokesch <daniel.prokesch@gmail.com>
 * Modified: Nga
 *
 * Desc: Common implementations for DECEN and HRC
 *
 * Core stream handling functions, including stream descriptors.
 *
 * A stream is the communication and synchronization primitive between two
 * tasks. If a task wants to use a stream, it must open it in order to
 * retrieve a stream descriptor. A task can open it for reading ('r',
 * a consumer) or writing ('w', a producer) but not both, meaning that
 * streams are uni-directional. A single stream can be opened by at most one
 * producer task and by at most one consumer task simultaneously.
 *
 * After opening, a consumer can read from a stream and a producer can write
 * to a stream, using the retrieved stream descriptor. Note that only the
 * streams are shared, not the stream descriptors.
 *
 * Within a stream a buffer struct is holding the actual data, which is
 * implemented as circular single-producer single-consumer in a concurrent,
 * lock-free manner.
 *
 * For synchronization between tasks, three blocking functions are provided:
 * LpelStreamRead() suspends the consumer trying to read from an empty stream,
 * LpelStreamWrite() suspends the producer trying to write to a full stream,
 * and a consumer can use LpelStreamPoll() to wait for the arrival of data
 * on any of the streams specified in a set.
 *
 *
 * @see http://www.cs.colorado.edu/department/publications/reports/docs/CU-CS-1023-07.pdf
 *      accessed Aug 26, 2010
 *      for more details on the FastForward queue.
 *
 *
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#include <lpel_common.h>

#include "arch/atomic.h"
#include "lpelcfg.h"
#include "buffer.h"
#include "task.h"

#include "stream.h"
#include "lpel/monitor.h"


/**
 * Store arbitrary user data in stream
 * CAUTION use at own risk
 */
void LpelStreamSetUsrData(lpel_stream_t *s, void *usr_data)
{
  s->usr_data = usr_data;
}

/**
 * Load user data from stream
 * CAUTION use at own risk
 */
void *LpelStreamGetUsrData(lpel_stream_t *s)
{
  return s->usr_data;
}



/**
 * Get the stream opened by a stream descriptor
 *
 * @param sd  the stream descriptor
 * @return    the stream opened by the stream descriptor
 */
lpel_stream_t *LpelStreamGet(lpel_stream_desc_t *sd)
{
  return sd->stream;
}



/**
 * Non-blocking, non-consuming read from a stream
 *
 * @param sd  stream descriptor
 * @return    the top item of the stream, or NULL if stream is empty
 */
void *LpelStreamPeek( lpel_stream_desc_t *sd)
{
  assert( sd->mode == 'r');
  return LpelBufferTop( sd->stream->buffer);
}

/**
 * Non-blocking write to a stream
 *
 * @param sd    stream descriptor
 * @param item  data item (a pointer) to write
 * @pre         current task is single writer
 * @pre         item != NULL
 * @return 0 if the item could be written, -1 if the stream was full
 */
int LpelStreamTryWrite( lpel_stream_desc_t *sd, void *item)
{
  if (!LpelBufferIsSpace(sd->stream->buffer)) {
    return -1;
  }
  LpelStreamWrite( sd, item );
  return 0;
}

/**
 * Poll a set of streams
 *
 * This is a blocking function called by a consumer which wants to wait
 * for arrival of data on any of a specified set of streams.
 * The consumer task is suspended while there is no new data on all streams.
 *
 * @param set     a stream descriptor set the task wants to poll
 * @pre           set must not be empty (*set != NULL)
 *
 * @post          The first element when iterating through the set after
 *                LpelStreamPoll() will be the one after the one which
 *                caused the task to wakeup,
 *                i.e., the first stream where data arrived.
 */
lpel_stream_desc_t *LpelStreamPoll( lpel_streamset_t *set)
{
  lpel_task_t *self;
  lpel_stream_iter_t *iter;
  int do_ctx_switch = 1;
  int cnt = 0;

  assert( *set != NULL);

  /* get 'self', i.e. the task calling LpelStreamPoll() */
  self = (*set)->task;

  iter = LpelStreamIterCreate( set);

  /* fast path*/
  while( LpelStreamIterHasNext( iter)) {
    lpel_stream_desc_t *sd = LpelStreamIterNext( iter);
    lpel_stream_t *s = sd->stream;
    if ( LpelBufferTop( s->buffer) != NULL) {
      LpelStreamIterDestroy(iter);
      *set = sd;
      return sd;
    }
  }


  /* place a poll token */
  atomic_store( &self->poll_token, 1);

  /* for each stream in the set */
  LpelStreamIterReset(iter, set);
  while( LpelStreamIterHasNext( iter)) {
    lpel_stream_desc_t *sd = LpelStreamIterNext( iter);
    lpel_stream_t *s = sd->stream;
    /* lock stream (prod-side) */
    PRODLOCK_LOCK( &s->prod_lock);
    { /* CS BEGIN */
      /* check if there is something in the buffer */
      if ( LpelBufferTop( s->buffer) != NULL) {
        /* yes, we can stop iterating through streams.
         * determine, if we have been woken up by another producer:
         */
        int tok = atomic_exchange( &self->poll_token, 0);
        if (tok) {
          /* we have not been woken yet, no need for ctx switch */
          do_ctx_switch = 0;
          self->wakeup_sd = sd;
        }
        /* unlock stream */
        PRODLOCK_UNLOCK( &s->prod_lock);
        /* exit loop */
        break;

      } else {
        /* nothing in the buffer, register stream as activator */
        s->is_poll = 1;
        cnt++;
        //sd->event_flags |= STDESC_WAITON;
        /* TODO marking all streams does potentially flood the log-files
           - is it desired to have anyway?
        MarkDirty( sd);
        */
      }
    } /* CS END */
    /* unlock stream */
    PRODLOCK_UNLOCK( &s->prod_lock);
  } /* end for each stream */

  /* context switch */
  if (do_ctx_switch) {
    /* set task as blocked */
    LpelTaskBlockStream( self);
  }
  assert( atomic_load( &self->poll_token) == 0);

  /* unregister activators
   * - would only be necessary, if the consumer task closes the stream
   *   while the producer is in an is_poll state,
   *   as this could result in a SEGFAULT when the producer
   *   is trying to dereference sd->stream->cons_sd
   * - a consumer closes the stream if it reads
   *   a terminate record or a sync record, and between reading the record
   *   and closing the stream the consumer issues no LpelStreamPoll()
   *   and no entity writes a record on the stream after these records.
   * UPDATE: with static/dynamc collectors in S-Net, this is possible!
   */
  LpelStreamIterReset(iter, set);
  while( LpelStreamIterHasNext( iter)) {
    lpel_stream_t *s = (LpelStreamIterNext(iter))->stream;
    PRODLOCK_LOCK( &s->prod_lock);
    s->is_poll = 0;
    PRODLOCK_UNLOCK( &s->prod_lock);
    if (--cnt == 0) break;
  }

  LpelStreamIterDestroy(iter);

  /* 'rotate' set to stream descriptor for non-empty buffer */
  *set = self->wakeup_sd;

  return self->wakeup_sd;
}

int LpelStreamGetId(lpel_stream_desc_t *sd) {
	if (sd)
		if (sd->stream)
			return sd->stream->uid;
	return -1;
}

/*
 * Return the number of items in the stream
 * n_sem may vary by 1
 * 	Case: consumer try to read, get blocked, n_sem = -1
 * 				producer writes, unblocks consumer, n_sem = 0
 * 	The variant is not important, here we reused n_sem
 * 	To get the accurate, one can introduce a new atomic counter and update when the stream is acctually written/read
 */
int LpelStreamFillLevel(lpel_stream_t *s) {
	if (s == NULL)
		return 0;
	int n = atomic_load(&s->n_sem);
	if (n < 0)
		return 0;
	return n;
}

