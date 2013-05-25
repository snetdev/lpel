#ifndef _STREAM_H_
#define _STREAM_H_

#include <pthread.h>
#include <lpel.h>
#include "decen_buffer.h"
#include "decen_task.h"
#include "lpel_main.h"

/* default size */
#ifndef  STREAM_BUFFER_SIZE
#define  STREAM_BUFFER_SIZE 16
#endif


/**
 * A stream which is shared between a
 * (single) producer and a (single) consumer.
 */
struct lpel_stream_t {
  buffer_t buffer;          /** buffer holding the actual data */
  unsigned int uid;         /** unique sequence number */
  PRODLOCK_TYPE prod_lock;  /** to support polling a lock is needed */
  int is_poll;              /** indicates if a consumer polls this stream,
                                is_poll is protected by the prod_lock */
  lpel_stream_desc_t *prod_sd;   /** points to the sd of the producer */
  lpel_stream_desc_t *cons_sd;   /** points to the sd of the consumer */
  atomic_int n_sem;           /** counter for elements in the stream */
  atomic_int e_sem;           /** counter for empty space in the stream */
  void *usr_data;           /** arbitrary user data */
};


#endif /* _STREAM_H_ */
