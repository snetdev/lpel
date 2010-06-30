#ifndef _STREAM_H_
#define _STREAM_H_

#ifndef  STREAM_BUFFER_SIZE
#define  STREAM_BUFFER_SIZE 32
#endif

#include "task.h"


/* 64bytes is the common size of a cache line */
#define longxCacheLine  (64/sizeof(long))

/* Padding is required to avoid false-sharing between core's private cache */
typedef struct {
  volatile unsigned long pread;
  volatile unsigned long cntread;
  long padding1[longxCacheLine-2];
  volatile unsigned long pwrite;
  volatile unsigned long cntwrite;
  long padding2[longxCacheLine-2];
  void *buf[STREAM_BUFFER_SIZE];
  task_t *producer;
  task_t *consumer;
} stream_t;


extern stream_t *StreamCreate(void);
extern void StreamDestroy(stream_t *s);
extern bool StreamOpen(stream_t *s, char mode);
extern void *StreamPeek(stream_t *s);
extern void *StreamRead(stream_t *s);
extern bool StreamIsSpace(stream_t *s);
extern void StreamWrite(stream_t *s, void *item);



#endif /* _STREAM_H_ */
