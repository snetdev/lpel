

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

#include "stream.h"
#include "bool.h"
#include "sysdep.h"

//TODO
#define assert(x) do { ; } while (0)

#ifndef  BUFFER_SIZE
#warning BUFFER_SIZE not defined, using default value
#define  BUFFER_SIZE 10
#endif

/* 64bytes is the common size of a cache line */
#define longxCacheLine  (64/sizeof(long))

/* Padding is required to avoid false-sharing between core's private cache */
struct stream {
  volatile unsigned long pread;
  long padding1[longxCacheLine-1];
  volatile unsigned long pwrite;
  long padding2[longxCacheLine-1];
  void *buf[BUFFER_SIZE];
  // TODO assignment/padding
  volatile bool *flag_written;
  volatile bool *flag_read;

};



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
    memset(&(s->buf), 0, BUFFER_SIZE*sizeof(void *));

    /* flag addresses not assigned */
    s->flag_written = NULL;
    s->flag_read = NULL;
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
 * Non-blocking read from a stream
 * - returns NULL if stream is empty
 */
void *StreamPeek(stream_t *s)
{ 
  /* TODO check if opened for reading */
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
  /* TODO check if opened for reading */

  /* wait while buffer is empty */
  while ( s->buf[s->pread] == NULL ) {
    /* WAIT on_write */;
  }

  item = s->buf[s->pread];
  s->buf[s->pread]=NULL;
  s->pread += (s->pread+1 >= BUFFER_SIZE) ? (1-BUFFER_SIZE) : 1;
  
  /* set the read flag */
  //TODO stream has to be opened for reading -> NULL check superfluous
  if (s->flag_read != NULL) { *s->flag_read = true; }

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
  /* TODO check if opened for writing */
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
  assert( item != NULL );
  /* TODO check if opened for writing */

  /* wait while buffer is full */
  while ( s->buf[s->pwrite] != NULL ) {
    /* WAIT on_read */;
  }

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
  
  /* set the written flag */
  //TODO stream has to be opened for writing -> NULL check superfluous
  if (s->flag_written != NULL) { *s->flag_written = true; }

  return;
}




