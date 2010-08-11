#ifndef _STREAM_H_
#define _STREAM_H_

#ifndef  STREAM_BUFFER_SIZE
#define  STREAM_BUFFER_SIZE 32
#endif

#include "task.h"
#include "spinlock.h"
#include "atomic.h"


/* 64bytes is the common size of a cache line */
#define longxCacheLine  (64/sizeof(long))

typedef struct stream stream_t;

/* Padding is required to avoid false-sharing between core's private cache */
struct stream {
  volatile unsigned long pread;
  long padding1[longxCacheLine-1];
  volatile unsigned long pwrite;
  long padding2[longxCacheLine-1];
  spinlock_t lock_prod;
  spinlock_t lock_cons;
  void *buf[STREAM_BUFFER_SIZE];
  unsigned long *cntread;
  unsigned long *cntwrite;
  task_t *producer;
  task_t *consumer;
  atomic_t refcnt;
};


extern stream_t *StreamCreate(void);
extern void StreamDestroy(stream_t *s);
extern bool StreamOpen(task_t *ct, stream_t *s, char mode);
extern void StreamClose(task_t *ct, stream_t *s);
extern void *StreamPeek(task_t *ct, stream_t *s);
extern void *StreamRead(task_t *ct, stream_t *s);
extern bool StreamIsSpace(task_t *ct, stream_t *s);
extern void StreamWrite(task_t *ct, stream_t *s, void *item);



#endif /* _STREAM_H_ */
