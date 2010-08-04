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
    s->pwrite = 0;
    /* clear all the buffer space */
    memset(&(s->buf), 0, STREAM_BUFFER_SIZE*sizeof(void *));

    s->cntread = NULL;
    s->cntwrite = NULL;
    
    /* producer/consumer not assigned */
    s->producer = NULL;
    s->consumer = NULL;

    /* refcnt reflects the number of tasks opened this stream {0,1,2}*/
    atomic_set(&s->refcnt, 1);
  }
  return s;
}


/**
 * Destroy a stream
 */
void StreamDestroy(stream_t *s)
{
  if ( fetch_and_dec(&s->refcnt) == 1 ) {
    if (s->producer != NULL) TaskDestroy(s->producer);
    if (s->consumer != NULL) TaskDestroy(s->consumer);

    free(s);
  }
}


/**
 * Open a stream for reading/writing
 *
 * @param ct  pointer to current task
 * @param s   stream to write to (not NULL)
 * @param mode  either 'r' for reading or 'w' for writing
 */
bool StreamOpen(task_t *ct, stream_t *s, char mode)
{
  /* increment reference counter of task */
  atomic_inc(&ct->refcnt);
  /* increment reference counter of stream */
  atomic_inc(&s->refcnt);

  switch(mode) {
  case 'w':
    assert( s->producer == NULL );
    s->producer = ct;

    /* add to tasks list of opened streams for writing (only for accounting)*/
    s->cntwrite = StreamtablePut(&ct->streamtab, s, mode);
    break;

  case 'r':
    assert( s->consumer == NULL );
    s->consumer = ct;

    /* add to tasks list of opened streams for reading (only for accounting)*/
    s->cntread = StreamtablePut(&ct->streamtab, s, mode);
    break;

  default:
    return false;
  }
  return true;
}

/**
 * Close a stream previously opened for reading/writing
 *
 * @param ct  pointer to current task
 * @param s   stream to write to (not NULL)
 */
void StreamClose(task_t *ct, stream_t *s)
{
  assert( ct == s->producer || ct == s->consumer );
  StreamtableMark(&ct->streamtab, s);
  StreamDestroy(s);
}


/**
 * Non-blocking read from a stream
 *
 * @return    NULL if stream is empty
 * @param ct  pointer to current task
 * @param s   stream to read from
 */
void *StreamPeek(task_t *ct, stream_t *s)
{ 
  /* check if opened for reading */
  assert( s->consumer == ct );

  /* if the buffer is empty, buf[pread]==NULL */
  return s->buf[s->pread];  
}    


/**
 * Blocking read from a stream
 *
 * Implementation note:
 * - modifies only pread pointer (not pwrite)
 *
 * @param ct  pointer to current task
 * @param s   stream to read from
 */
void *StreamRead(task_t *ct, stream_t *s)
{
  void *item;

  /* check if opened for reading */
  assert( s->consumer == ct );

  /* wait while buffer is empty */
  while ( s->buf[s->pread] == NULL ) {
    TaskWaitOnWrite(ct);
  }

  /* READ FROM BUFFER */
  item = s->buf[s->pread];
  s->buf[s->pread]=NULL;
  s->pread += (s->pread+1 >= STREAM_BUFFER_SIZE) ?
              (1-STREAM_BUFFER_SIZE) : 1;
  *s->cntread++;
  
  /* signal the producer a read event */
  if (s->producer != NULL) { s->producer->ev_read = 1; }

  return item;
}


/**
 * Check if there is space in the buffer
 *
 * A writer can use this function before a write
 * to ensure the write succeeds (without blocking)
 *
 * @param ct  pointer to current task
 * @param s   stream opened for writing
 */
bool StreamIsSpace(task_t *ct, stream_t *s)
{
  /* check if opened for writing */
  assert( ct == NULL ||  s->producer == ct );

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
 *
 * @param ct  pointer to current task
 * @param s   stream to write to
 * @param item  data item (a pointer) to write
 */
void StreamWrite(task_t *ct, stream_t *s, void *item)
{
  /* check if opened for writing */
  assert( ct == NULL ||  s->producer == ct );

  assert( item != NULL );

  /* wait while buffer is full */
  while ( s->buf[s->pwrite] != NULL ) {
    TaskWaitOnRead(ct);
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
  *s->cntwrite++;
  
  /* signal the consumer a write event */
  if (s->consumer != NULL) { s->consumer->ev_write = 1; }

  return;
}




