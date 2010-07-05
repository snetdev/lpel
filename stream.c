/*
 * This file is based on the implementation of FastFlow
 */


/* 
 * Single-Writer Single-Reader circular buffer.
 * No lock is needed around pop and push methods.
 * 
 * A NULL value is used to indicate buffer full and 
 * buffer empty conditions.
 *
 */

#include <malloc.h>
#include <string.h>
#include <assert.h>

#include "stream.h"

#include "lpel.h"
#include "task.h"
#include "sysdep.h"


/**
 * Create a stream
 */
stream_t *StreamCreate(void)
{
  stream_t *s = (stream_t *) malloc( sizeof(stream_t) );
  if (s != NULL) {
    s->pread = 0;
    s->cntread = 0;
    s->pwrite = 0;
    s->cntwrite = 0;
    /* clear all the buffer space */
    memset(&(s->buf), 0, STREAM_BUFFER_SIZE*sizeof(void *));

    /* producer/consumer not assigned */
    s->producer = NULL;
    s->consumer = NULL;
  }
  return s;
}


/**
 * Destroy a stream
 */
void StreamDestroy(stream_t *s)
{
  free(s);
  s = NULL;
}


/**
 * Open a stream for reading/writing
 * - s != NULL
 * - mode: either 'w' or 'r'
 */
bool StreamOpen(stream_t *s, char mode)
{
  task_t *ct = LpelGetCurrentTask();
  switch(mode) {
  case 'w':
    assert( s->producer == NULL );
    s->producer = ct;

    /* add to tasks list of opened streams for writing (only for accounting)*/
    SetAdd(&ct->streams_writing, s);
    break;

  case 'r':
    assert( s->consumer == NULL );
    s->consumer = ct;

    /* add to tasks list of opened streams for reading (only for accounting)*/
    SetAdd(&ct->streams_reading, s);
    break;

  default:
    return false;
  }
  return true;
}


/**
 * Non-blocking read from a stream
 * - returns NULL if stream is empty
 */
void *StreamPeek(stream_t *s)
{ 
  /* check if opened for reading */
  assert( s->consumer == LpelGetCurrentTask() );

  /* if the buffer is empty, buf[pread]==NULL */
  return s->buf[s->pread];  
}    


/**
 * Blocking read from a stream
 *
 * Implementation note:
 * - modifies only pread pointer (not pwrite)
 */
void *StreamRead(stream_t *s)
{
  void *item;

  /* check if opened for reading */
  assert( s->consumer == LpelGetCurrentTask() );

  /* wait while buffer is empty */
  while ( s->buf[s->pread] == NULL ) {
    TaskWaitOnWrite();
  }

  /* READ FROM BUFFER */
  item = s->buf[s->pread];
  s->buf[s->pread]=NULL;
  s->pread += (s->pread+1 >= STREAM_BUFFER_SIZE) ?
              (1-STREAM_BUFFER_SIZE) : 1;
  s->cntread++;
  
  /* signal the producer a read event */
  if (s->producer != NULL) { s->producer->ev_read = true; }

  return item;
}


/**
 * Check if there is space in the buffer
 *
 * A writer can use this function before a write
 * to ensure the write succeeds (without blocking)
 */
bool StreamIsSpace(stream_t *s)
{
  /* check if opened for writing */
  assert( s->producer == LpelGetCurrentTask() );

  /* if there is space in the buffer, the location at pwrite holds NULL */
  return ( s->buf[s->pwrite] == NULL );
}


/**
 * Blocking write to a stream
 *
 * Precondition: item != NULL
 *
 * Implementation note:
 * - modifies only pwrite pointer (not pread)
 */
void StreamWrite(stream_t *s, void *item)
{
  /* check if opened for writing */
  assert( s->producer == LpelGetCurrentTask() );

  assert( item != NULL );

  /* wait while buffer is full */
  while ( s->buf[s->pwrite] != NULL ) {
    TaskWaitOnRead();
  }

  /* WRITE TO BUFFER */
  /* Write Memory Barrier: ensure all previous memory write 
   * are visible to the other processors before any later
   * writes are executed.  This is an "expensive" memory fence
   * operation needed in all the architectures with a weak-ordering 
   * memory model where stores can be executed out-or-order 
   * (e.g. PowerPC). This is a no-op on Intel x86/x86-64 CPUs.
   */
  WMB(); 
  s->buf[s->pwrite] = item;
  s->pwrite += (s->pwrite+1 >= STREAM_BUFFER_SIZE) ?
               (1-STREAM_BUFFER_SIZE) : 1;
  s->cntwrite++;
  
  /* signal the consumer a write event */
  if (s->consumer != NULL) { s->consumer->ev_write = true; }

  return;
}




