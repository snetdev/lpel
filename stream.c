/*
 * This file was based on the implementation of FastFlow.
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

    /* producer/consumer not assigned */
    s->prod.task = s->cons.task = NULL;
    s->prod.tbe = s->cons.tbe = NULL;
    spinlock_init(&s->prod.lock);
    spinlock_init(&s->cons.lock);

    /* refcnt reflects the number of tasks
       + the stream itself opened this stream {1,2,3} */
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
  /* increment reference counter of stream */
  atomic_inc(&s->refcnt);

  switch(mode) {
  case 'w':
    spinlock_lock(&s->prod.lock);
    {
      assert( s->prod.task == NULL );
      s->prod.task = ct;
      s->prod.tbe  = StreamsetAdd(ct->streams_write, s, NULL);
    }
    spinlock_unlock(&s->prod.lock);
    break;

  case 'r':
    spinlock_lock(&s->cons.lock);
    {
      int grpidx;
      assert( s->cons.task == NULL );
      s->cons.task = ct;
      s->cons.tbe  = StreamsetAdd(ct->streams_read, s, &grpidx);

      /*TODO if consumer task is a collector, register flagtree,
        set the flag if stream not empty? */
    }
    spinlock_unlock(&s->cons.lock);
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
  if ( ct == s->prod.task ) {
    spinlock_lock(&s->prod.lock);
    {
      StreamsetRemove( ct->streams_write, s->prod.tbe );
      s->prod.task = NULL;
      s->prod.tbe = NULL;
    }
    spinlock_unlock(&s->prod.lock);

  } else if ( ct == s->cons.task ) {
    spinlock_lock(&s->cons.lock);
    {
      StreamsetRemove( ct->streams_read, s->cons.tbe );
      s->cons.task = NULL;
      s->cons.tbe = NULL;
      /*TODO if consumer was collector, unregister flagtree */
    }
    spinlock_unlock(&s->cons.lock);
  
  } else {
    /* task is neither producer nor consumer:
       something went wrong */
    assert(0);
  }
  
  /* destroy request */
  StreamDestroy(s);
}


/**
 * Non-blocking read from a stream
 *
 * @param ct  pointer to current task
 * @pre       current task is single reader
 * @param s   stream to read from
 * @return    NULL if stream is empty
 */
void *StreamPeek(task_t *ct, stream_t *s)
{ 
  /* check if opened for reading */
  assert( s->cons.task == ct );

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
 * @pre       current task is single reader
 */
void *StreamRead(task_t *ct, stream_t *s)
{
  void *item;

  /* check if opened for reading */
  assert( s->cons.task == ct );

  /* wait if buffer is empty */
  if ( s->buf[s->pread] != NULL ) {
    TaskWaitOnWrite(ct, s);
  }

  /* READ FROM BUFFER */
  item = s->buf[s->pread];
  s->buf[s->pread]=NULL;
  s->pread += (s->pread+1 >= STREAM_BUFFER_SIZE) ?
              (1-STREAM_BUFFER_SIZE) : 1;

  /* for monitoring */
  StreamsetEvent( ct->streams_read, s->cons.tbe );

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
 * @pre       current task is single writer
 */
bool StreamIsSpace(task_t *ct, stream_t *s)
{
  /* check if opened for writing */
  assert( ct == NULL ||  s->prod.task == ct );

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
 * @param ct    pointer to current task
 * @param s     stream to write to
 * @param item  data item (a pointer) to write
 * @pre         current task is single writer
 * @pre         item != NULL
 */
void StreamWrite(task_t *ct, stream_t *s, void *item)
{
  /* check if opened for writing */
  assert( ct == NULL ||  s->prod.task == ct );

  assert( item != NULL );

  /* wait while buffer is full */
  if ( s->buf[s->pwrite] == NULL ) {
    TaskWaitOnRead(ct, s);
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

  /* for monitoring */
  StreamsetEvent( ct->streams_read, s->cons.tbe );

/* signal the consumer a write event */
  spinlock_lock(&s->cons.lock);
  /* TODO if flagtree registered, use flagtree mark */
  spinlock_unlock(&s->cons.lock);

  return;
}

