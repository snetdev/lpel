#ifndef _HRC_STREAM_H
#define _HRC_STREAM_H

#include <hrc_lpel.h>
#include "stream.h"


/*
 * scheduling information from stream
 */
struct stream_sched_info_t {
	int is_entry;							/* if stream is an entry stream, i.e. written by source */
	int is_exit;							/* if stream is an exit stream, i.e. read by sink */
	int read;
	int write;
};

lpel_task_t *LpelStreamProducer(lpel_stream_t *s);
lpel_task_t *LpelStreamConsumer(lpel_stream_t *s);
int LpelStreamIsEntry(lpel_stream_t *s);
int LpelStreamIsExit(lpel_stream_t *s);
int LpelStreamFillLevel(lpel_stream_t *s);
#endif /* HRC_STREAM_H */ 
