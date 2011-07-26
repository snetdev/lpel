#ifndef _STREAM_H_
#define _STREAM_H_


#include "lpel_name.h"
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
typedef struct lpel_stream_desc_t lpel_stream_desc_t;

struct lpel_stream_desc_t {
  lpel_task_t   *task;        /** the task which opened the stream */
  lpel_stream_t *stream;      /** pointer to the stream */
  char mode;                  /** either 'r' or 'w' */
  struct lpel_stream_desc_t *next; /** for organizing in stream sets */
  struct mon_stream_t *mon;   /** monitoring object */
};




lpel_stream_t *LPEL_EXPORT(StreamCreate)( int);
void LPEL_EXPORT(StreamDestroy)( lpel_stream_t *s);

lpel_stream_desc_t *
LPEL_EXPORT(StreamOpen)( lpel_stream_t *s, char mode);

void  LPEL_EXPORT(StreamClose)(    lpel_stream_desc_t *sd, int destroy_s);
void  LPEL_EXPORT(StreamReplace)(  lpel_stream_desc_t *sd, lpel_stream_t *snew);
void *LPEL_EXPORT(StreamPeek)(     lpel_stream_desc_t *sd);
void *LPEL_EXPORT(StreamRead)(     lpel_stream_desc_t *sd);
void  LPEL_EXPORT(StreamWrite)(    lpel_stream_desc_t *sd, void *item);
int   LPEL_EXPORT(StreamTryWrite)( lpel_stream_desc_t *sd, void *item);

lpel_stream_t *LPEL_EXPORT(StreamGet)(lpel_stream_desc_t *sd);

lpel_stream_desc_t *LPEL_EXPORT(StreamPoll)( lpel_stream_desc_t **set);



#endif /* _STREAM_H_ */
