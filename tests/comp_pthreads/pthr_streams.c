

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

#include "pthr_streams.h"


struct pthr_stream_t {
  pthread_mutex_t lock;
  pthread_cond_t  notempty;
  pthread_cond_t  notfull;
  
  void *buffer[PTHR_STREAM_BUFFER_SIZE];
  int head, tail;
  int count;
};


struct pthr_stream_desc_t {
  pthr_stream_t *stream;
  struct pthr_stream_desc_t *next;
};





pthr_stream_t *PthrStreamCreate(void)
{
  pthr_stream_t *s = malloc(sizeof(pthr_stream_t));
  pthread_mutex_init(&s->lock, NULL);
  pthread_cond_init(&s->notempty, NULL);
  pthread_cond_init(&s->notfull, NULL);
  
  s->head = 0;
  s->tail = 0;
  s->count = 0;

  memset( s->buffer, 0, PTHR_STREAM_BUFFER_SIZE*sizeof(void*) );
  return s;
}

void PthrStreamDestroy(pthr_stream_t *s)
{
  pthread_mutex_destroy(&s->lock);
  pthread_cond_destroy(&s->notempty);
  pthread_cond_destroy(&s->notfull);

  free(s);
}


pthr_stream_desc_t *PthrStreamOpen(pthr_stream_t *stream, char mode)
{
  pthr_stream_desc_t *sd = malloc(sizeof(pthr_stream_t));
  sd->stream = stream;
  sd->next = NULL;

  assert( mode=='w' || mode=='r' );
  /* mode is ignored in this implementation */

  return sd;
}


void PthrStreamClose(pthr_stream_desc_t *sd, int destroy_s)
{
  if (destroy_s) {
    free(sd->stream);
  }
  free(sd);
}


void PthrStreamWrite(pthr_stream_desc_t *sd, void *item)
{
  pthr_stream_t *s = sd->stream;

  pthread_mutex_lock( &s->lock);
  while(s->count == PTHR_STREAM_BUFFER_SIZE){
    pthread_cond_wait(&s->notfull, &s->lock);
  }
 
  s->buffer[s->tail] = item;
  s->tail = (s->tail+1) % PTHR_STREAM_BUFFER_SIZE;
  s->count++;

  if (s->count==1) pthread_cond_signal(&s->notempty);

  pthread_mutex_unlock( &s->lock);
}

void *PthrStreamRead(pthr_stream_desc_t *sd)
{
  pthr_stream_t *s = sd->stream;
  void *item;

  pthread_mutex_lock( &s->lock);
  while(s->count == 0){
    pthread_cond_wait(&s->notempty, &s->lock);
  }
 
  item = s->buffer[s->head];
  s->buffer[s->head] = NULL;
  s->head = (s->head+1) % PTHR_STREAM_BUFFER_SIZE;
  s->count--;

  if (s->count == PTHR_STREAM_BUFFER_SIZE-1) pthread_cond_signal(&s->notfull);

  pthread_mutex_unlock( &s->lock);

  return item;
}


