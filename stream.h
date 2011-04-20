#ifndef _STREAM_H_
#define _STREAM_H_



#include "task.h"


#ifndef  STREAM_BUFFER_SIZE
#define  STREAM_BUFFER_SIZE 16
#endif

//#define STREAM_POLL_SPINLOCK

/** stream type */
typedef struct lpel_stream_t lpel_stream_t;

/**
 * A stream descriptor
 *
 * A producer/consumer must open a stream before using it, and by opening
 * a stream, a stream descriptor is created and returned.
 */
typedef struct lpel_stream_desc_t {
  lpel_task_t   *task;        /** the task which opened the stream */
  lpel_stream_t *stream;      /** pointer to the stream */
  char mode;                  /** either 'r' or 'w' */
  struct lpel_stream_desc_t *next; /** for organizing in stream lists */
  struct mon_stream_t *mon;   /** monitoring object */
} lpel_stream_desc_t;


lpel_stream_t *LpelStreamCreate( int);
void LpelStreamDestroy( lpel_stream_t *s);

lpel_stream_desc_t *
LpelStreamOpen( lpel_stream_t *s, char mode);

void  LpelStreamClose(    lpel_stream_desc_t *sd, int destroy_s);
void  LpelStreamReplace(  lpel_stream_desc_t *sd, lpel_stream_t *snew);
void *LpelStreamPeek(     lpel_stream_desc_t *sd);
void *LpelStreamRead(     lpel_stream_desc_t *sd);
void  LpelStreamWrite(    lpel_stream_desc_t *sd, void *item);
int   LpelStreamTryWrite( lpel_stream_desc_t *sd, void *item);

lpel_stream_t *LpelStreamGet(lpel_stream_desc_t *sd);

lpel_stream_desc_t *LpelStreamPoll( lpel_stream_desc_t **set);



#endif /* _STREAM_H_ */
