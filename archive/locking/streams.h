
#ifndef _STREAMS_H_
#define _STREAMS_H_

#include "bool.h"


#ifndef BUFFER_SIZE
#warning "BUFFER_SIZE is not defined, using default value"
#define BUFFER_SIZE 10

#endif


typedef struct stream stream_t;
typedef struct streamset streamset_t;

typedef void * item_t;


extern stream_t *StreamCreate(void);
extern void StreamDestroy(stream_t *s);
extern item_t StreamRead(stream_t *s);
extern item_t StreamPeek(stream_t *s);
extern void StreamWrite(stream_t *s, item_t item);
extern unsigned int StreamCount(stream_t *s);

extern streamset_t *StreamsetCreate(void);
extern void StreamsetDestroy(streamset_t *set);
extern void StreamsetAdd(streamset_t *set, stream_t *s);
extern void StreamsetRemove(streamset_t *set, stream_t *s);
extern struct pair StreamsetPeekMany(streamset_t *set);



struct stream {
  item_t buffer[BUFFER_SIZE];
  unsigned int start;
  unsigned int end;
  unsigned int count;
  pthread_mutexattr_t lockattr;
  pthread_mutex_t lock;
  /* condition variables */
  bool notempty;
  bool notfull;
  streamset_t *set;
  stream_t *next;
};



struct streamset {
  stream_t *hook;
  unsigned int count_streams;
  unsigned int count_input;
  unsigned int count_new;
  pthread_mutexattr_t lockattr;
  pthread_mutex_t lock;
};


struct pair {
  stream_t *s;
  void *item;
};

#define LOCK(lock) \
  if (pthread_mutex_lock( &(lock) ) != 0) { \
    perror("stream lock failed"); \
    exit(-1); \
  }

#define UNLOCK(lock) \
  if (pthread_mutex_unlock( &(lock) ) != 0) { \
    perror("stream unlock failed"); \
    exit(-1); \
  }


#endif /* _STREAMS_H_ */
