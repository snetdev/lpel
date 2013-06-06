#ifndef _HRC_TASK_H_
#define _HRC_TASK_H_


#include <hrc_lpel.h>
#include "arch/mctx.h"


#include "arch/atomic.h"

#define _USE_NEG_DEMAND_LIMIT_

#define NEG_DEMAND_LIMIT 16


/**
 * If a task size <= 0 is specified,
 * use the default size
 */
#define LPEL_TASK_SIZE_DEFAULT  8192  /* 8k */

struct workerctx_t;
struct mon_task_t;

struct stream_elem_t {
	struct lpel_stream_desc_t *stream_desc;
	struct stream_elem_t *next;
};

typedef struct stream_elem_t stream_elem_t;

typedef struct {
	int rec_cnt;
	int rec_limit_factor;
	int rec_limit;
	double prior;
	stream_elem_t *in_streams;
	stream_elem_t *out_streams;
} sched_task_t;


/**
 * TASK CONTROL BLOCK
 */
struct lpel_task_t {
  /** intrinsic pointers for organizing tasks in a list*/
  struct lpel_task_t *prev, *next;
  unsigned int uid;    /** unique identifier */
  lpel_taskstate_t state;   /** state */
  int wakedup;						/** to keep track that the task has been waked up before returned */

  struct workerctx_t *worker_context;  /** worker context for this task */

  /**
   * indicates the SD which points to the stream which has new data
   * and caused this task to be woken up
   */
  struct lpel_stream_desc_t *wakeup_sd;
  atomic_int poll_token;        /** poll token, accessed concurrently */

  /* ACCOUNTING INFORMATION */
  struct mon_task_t *mon;

  /* CODE */
  int size;             /** complete size of the task, incl stack */
  mctx_t mctx;          /** machine context of the task*/
  lpel_taskfunc_t func; /** function of the task */
  void *inarg;          /** input argument  */
  void *outarg;         /** output argument  */

  /* info supporting scheduling */
  sched_task_t sched_info;
};


void LpelTaskDestroy(lpel_task_t *t);
void LpelTaskBlockStream(lpel_task_t *ct);
void LpelTaskUnblock(lpel_task_t *t);
int LpelTaskIsWrapper(lpel_task_t *);

/******* DYNAMIC PRIORITY BASED ON THE STREAM FILL LEVEL ***********/
/**
 * check and yield if it need to
 * the yield condition depends on the implementation of the scheduler; e.g. task has processed a limited number of data record
 */
void LpelTaskCheckYield(lpel_task_t *t);

void LpelTaskSetPrior(lpel_task_t *t, double p);

void LpelTaskAddStream(lpel_task_t *t, lpel_stream_desc_t *des, char mode);
void LpelTaskRemoveStream(lpel_task_t *t, lpel_stream_desc_t *des, char mode);
double LpelTaskCalPriority(lpel_task_t *t);


#endif
