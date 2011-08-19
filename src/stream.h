#ifndef _STREAM_H_
#define _STREAM_H_

#include <lpel.h>

#include "task.h"

/* default size */
#ifndef  STREAM_BUFFER_SIZE
#define  STREAM_BUFFER_SIZE 16
#endif

//#define STREAM_POLL_SPINLOCK



struct lpel_stream_desc_t {
  lpel_task_t   *task;        /** the task which opened the stream */
  lpel_stream_t *stream;      /** pointer to the stream */
  char mode;                  /** either 'r' or 'w' */
  struct lpel_stream_desc_t *next; /** for organizing in stream sets */
  struct mon_stream_t *mon;   /** monitoring object */
};



#endif /* _STREAM_H_ */
