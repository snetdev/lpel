

#include <pthread.h>

#define PTHR_STREAM_BUFFER_SIZE 10

typedef struct {
  pthread_mutex_t lock;
  pthread_cond_t  notempty;
  pthread_cond_t  notfull;
  
  void *buffer[PTHR_STREAM_BUFFER_SIZE];
  int head, tail;
  int count;
} pthr_stream_t;


pthr_stream_t *PthrStreamCreate(void);
void PthrStreamDestroy(pthr_stream_t *s);
void PthrStreamWrite(pthr_stream_t *s, void *item);
void *PthrStreamRead(pthr_stream_t *s);
