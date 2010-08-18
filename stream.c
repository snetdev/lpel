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
#include "flagtree.h"
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
    s->wany_idx = -1;
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
      
      /*if consumer task is a collector, register flagtree */
      if ( TASK_IS_WAITANY(ct) ) {
        if (grpidx > ct->max_grp_idx) {
          FlagtreeGrow(&ct->flagtree);
          ct->max_grp_idx *= 2;
        }
        s->wany_idx = grpidx;
        /* if stream not empty, mark flagtree */
        if (s->buf[s->pread] != NULL) {
          FlagtreeMark(&ct->flagtree, s->wany_idx, -1);
        }
      }
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
      
      /* if consumer was collector, unregister flagtree */
      if ( TASK_IS_WAITANY(ct) ) {
        s->wany_idx = -1;
      }
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
 * Replace a stream opened for reading by another stream
 *
 * This is like calling StreamClose(s); StreamOpen(snew, 'r');
 * sequentially, but with the difference that the "position"
 * of the old stream is used for the new stream.
 * This is used in collector tasks, i.e., tasks that can wait
 * on input from any of its streams opened for reading.
 *
 * @param ct    current task
 * @param s     stream which to replace
 * @param snew  stream that replaces s
 */
void StreamReplace(task_t *ct, stream_t *s, stream_t *snew)
{
  /* only streams opened for reading can be replaced */
  assert( ct == s->cons.task );
  /* new stream must have been closed by previous consumer */
  assert( snew->cons.task == NULL );

  spinlock_lock(&snew->cons.lock);
  {
    /* Task has opened s, hence s contains a reference to the tbe.
       In that tbe, the stream must be replaced. Then, the reference
       to the tbe must be copied to the new stream */
    StreamsetReplace( ct->streams_read, s->cons.tbe, snew );
    snew->cons.tbe = s->cons.tbe;
    /* also, set the task in the new stream */
    snew->cons.task = ct; /* ct == s->cons.task */

    /*if consumer is collector, register flagtree for new stream */
    snew->wany_idx = s->wany_idx;
    /* if stream not empty, mark flagtree */
    if (snew->wany_idx >= 0 && snew->buf[s->pread] != NULL) {
      FlagtreeMark(&ct->flagtree, snew->wany_idx, -1);
    }
  }
  spinlock_unlock(&snew->cons.lock);

  /* clear references in old stream */
  spinlock_lock(&s->cons.lock);
  {
    s->cons.task = NULL;
    s->cons.tbe = NULL;
    s->wany_idx = -1;
  }
  spinlock_unlock(&s->cons.lock);

  /* destroy request for old stream */
  StreamDestroy(s);
}

/*
 * TODO stream_iter_t *StreamWaitAny(task_t *ct)
 */




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
  if ( s->buf[s->pread] == NULL ) {
    TaskWaitOnWrite(ct, s);
    assert( s->buf[s->pread] != NULL );
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
  if ( s->buf[s->pwrite] != NULL ) {
    TaskWaitOnRead(ct, s);
    assert( s->buf[s->pwrite] == NULL );
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
  if (ct!=NULL) StreamsetEvent( ct->streams_read, s->cons.tbe );

  spinlock_lock(&s->cons.lock);
  /*  if flagtree registered, use flagtree mark */
  if (s->wany_idx >= 0) {
    FlagtreeMark(
        &s->cons.task->flagtree,
        s->wany_idx,
        s->cons.task->owner
        );
  }
  spinlock_unlock(&s->cons.lock);

  return;
}

