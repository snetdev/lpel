#ifndef _HRC_STREAM_H
#define _HRC_STREAM_H

#include <hrc_lpel.h>
#include "lpel_main.h"
#include "hrc_buffer.h"

/* default size */
#ifndef  STREAM_BUFFER_SIZE
#define  STREAM_BUFFER_SIZE 16
#endif

typedef enum {
	LPEL_STREAM_ENTRY,
	LPEL_STREAM_EXIT,
	LPEL_STREAM_MIDDLE
} lpel_stream_type;


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
  struct lpel_stream_t *next;	/* to organize stream in the free list */
  lpel_stream_type type;			/* stream type (entry/exit/middle */
  int read_cnt;								/* read counter, to calculate fill level */
  int write_cnt;							/* write counter, to calculate fill level */
};


int LpelStreamFillLevel(lpel_stream_t *s);
lpel_task_t *LpelStreamConsumer(lpel_stream_t *s);
lpel_task_t *LpelStreamProducer(lpel_stream_t *s);

#endif /* _STREAM_H_ */
