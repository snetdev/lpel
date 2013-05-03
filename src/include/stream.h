#ifndef _STREAM_H_
#define _STREAM_H_

#include <pthread.h>
#include <lpel_common.h>
#include "buffer.h"
#include "task.h"

/* default size */
#ifndef  STREAM_BUFFER_SIZE
#define  STREAM_BUFFER_SIZE 16
#endif

//#define STREAM_POLL_SPINLOCK

/** Macros for lock handling */

#ifdef STREAM_POLL_SPINLOCK

#define PRODLOCK_TYPE       pthread_spinlock_t
#define PRODLOCK_INIT(x)    pthread_spin_init(x, PTHREAD_PROCESS_PRIVATE)
#define PRODLOCK_DESTROY(x) pthread_spin_destroy(x)
#define PRODLOCK_LOCK(x)    pthread_spin_lock(x)
#define PRODLOCK_UNLOCK(x)  pthread_spin_unlock(x)

#else

#define PRODLOCK_TYPE       pthread_mutex_t
#define PRODLOCK_INIT(x)    pthread_mutex_init(x, NULL)
#define PRODLOCK_DESTROY(x) pthread_mutex_destroy(x)
#define PRODLOCK_LOCK(x)    pthread_mutex_lock(x)
#define PRODLOCK_UNLOCK(x)  pthread_mutex_unlock(x)

#endif /* STREAM_POLL_SPINLOCK */


struct lpel_stream_desc_t {
  lpel_task_t   *task;        /** the task which opened the stream */
  lpel_stream_t *stream;      /** pointer to the stream */
  char mode;                  /** either 'r' or 'w' */
  struct lpel_stream_desc_t *next; /** for organizing in stream sets */
  struct mon_stream_t *mon;   /** monitoring object */
};



/**
 * A stream which is shared between a
 * (single) producer and a (single) consumer.
 */
struct lpel_stream_t {
  buffer_t *buffer;          /** buffer holding the actual data */
  unsigned int uid;         /** unique sequence number */
  PRODLOCK_TYPE prod_lock;  /** to support polling a lock is needed */
  int is_poll;              /** indicates if a consumer polls this stream,
                                is_poll is protected by the prod_lock */
  lpel_stream_desc_t *prod_sd;   /** points to the sd of the producer */
  lpel_stream_desc_t *cons_sd;   /** points to the sd of the consumer */
  atomic_int n_sem;           /** counter for elements in the stream */
  atomic_int e_sem;           /** counter for empty space in the stream */
  void *usr_data;           /** arbitrary user data */

  int is_entry;		/* if stream is an entry stream, used to support source/sink */
  int is_exit;			/* if stream is an exit stream, used to support source/sink */
};


int LpelStreamFillLevel(lpel_stream_t *s);
lpel_task_t *LpelStreamConsumer(lpel_stream_t *s);
lpel_task_t *LpelStreamProducer(lpel_stream_t *s);

#endif /* _STREAM_H_ */
