

/*
 * This file is adopted from fastflow
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

#include "lpel_private.h"

#include "sysdep.h"


#ifndef  BUFFER_SIZE
#warning BUFFER_SIZE not defined, using default value
#define  BUFFER_SIZE 10
#endif

/* 64bytes is the common size of a cache line */
#define longxCacheLine  (64/sizeof(long))

/* Padding is required to avoid false-sharing between core's private cache */
struct stream {
  volatile unsigned long pread;
  volatile unsigned long cntread;
  long padding1[longxCacheLine-2];
  volatile unsigned long pwrite;
  volatile unsigned long cntwrite;
  long padding2[longxCacheLine-2];
  void *buf[BUFFER_SIZE];
  task_t *producer;
  task_t *consumer;
};



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
    memset(&(s->buf), 0, BUFFER_SIZE*sizeof(void *));

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
  task_t *current_task = LpelGetCurrentTask();
  switch(mode) {
  case 'w':
    assert( s->producer == NULL );
    s->producer = current_task;
    break;
  case 'r':
    assert( s->consumer == NULL );
    s->consumer = current_task;
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
  task_t *t = LpelGetCurrentTask();

  /* check if opened for reading */
  assert( s->consumer == t );

  /* wait while buffer is empty */
  while ( s->buf[s->pread] == NULL ) {
    /* WAIT on write event*/
    t->event_ptr = &t->ev_write;
    t->state = TASK_WAITING;
    /* context switch */
    co_resume();
  }

  /* READ FROM BUFFER */
  item = s->buf[s->pread];
  s->buf[s->pread]=NULL;
  s->pread += (s->pread+1 >= BUFFER_SIZE) ? (1-BUFFER_SIZE) : 1;
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
  task_t *t = LpelGetCurrentTask();

  /* check if opened for writing */
  assert( s->flag_read == &t->ev_read );

  assert( item != NULL );

  /* wait while buffer is full */
  while ( s->buf[s->pwrite] != NULL ) {
    /* WAIT on read event*/;
    t->event_ptr = &t->ev_read;
    t->state = TASK_WAITING;
    /* context switch */
    co_resume();
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
  s->pwrite += (s->pwrite+1 >= BUFFER_SIZE) ? (1-BUFFER_SIZE) : 1;
  s->cntwrite++;
  
  /* signal the consumer a write event */
  if (s->consumer != NULL) { s->consumer->ev_write = true; }

  return;
}




