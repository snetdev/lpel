

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

#include "pthr_streams.h"



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

void PthrStreamWrite(pthr_stream_t *s, void *item)
{
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

void *PthrStreamRead(pthr_stream_t *s)
{
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


