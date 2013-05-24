#ifndef _HRC_STREAM_H
#define _HRC_STREAM_H

#include <hrc_lpel.h>


/* default size */
#ifndef  STREAM_BUFFER_SIZE
#define  STREAM_BUFFER_SIZE 16
#endif

//#define STREAM_POLL_SPINLOCK

int LpelStreamFillLevel(lpel_stream_t *s);
lpel_task_t *LpelStreamConsumer(lpel_stream_t *s);
lpel_task_t *LpelStreamProducer(lpel_stream_t *s);

#endif /* _STREAM_H_ */
