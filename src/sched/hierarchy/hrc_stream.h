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
	int destroy;							/* if snet-rts level notify to destroy the stream */
	int ref_count;						/* reference counter = number of stream_descriptors still linking to the stream
															stream is destroyed when it is notified from snet-rts and the ref_count = 0 */
	PRODLOCK_TYPE sd_lock;		/* lock to update ref_count, destroy and stream_descriptors */
	};

lpel_task_t *LpelStreamProducer(lpel_stream_t *s);
lpel_task_t *LpelStreamConsumer(lpel_stream_t *s);
int LpelStreamIsEntry(lpel_stream_t *s);
int LpelStreamIsExit(lpel_stream_t *s);

#endif /* HRC_STREAM_H */ 
