#ifndef _HRC_TASK_H_
#define _HRC_TASK_H_


#include <hrc_lpel.h>
#include <task.h>
#include "arch/mctx.h"


#include "arch/atomic.h"


/**
 * If a task size <= 0 is specified,
 * use the default size
 */
#define LPEL_TASK_SIZE_DEFAULT  8192  /* 8k */

#define LPEL_REC_LIMIT_DEFAULT 	8

struct workerctx_t;
struct mon_task_t;

struct stream_elem_t {
	struct lpel_stream_desc_t *stream_desc;
	struct stream_elem_t *next;
};

typedef struct stream_elem_t stream_elem_t;

struct sched_task_t{
	int rec_cnt;
	int rec_limit;
	double prior;
	stream_elem_t *in_streams;
	stream_elem_t *out_streams;
};

double LpelTaskCalPriority(lpel_task_t *t);
void LpelTaskCheckYield(lpel_task_t *t);

#endif /* _HRC_TASK_H */
