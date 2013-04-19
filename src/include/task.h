#ifndef _TASK_H_
#define _TASK_H_


#include <lpel.h>

#include "arch/mctx.h"


#include "arch/atomic.h"

/**
 * If a task size <= 0 is specified,
 * use the default size
 */
#define LPEL_TASK_SIZE_DEFAULT  8192  /* 8k */

#define TASK_STACK_ALIGN  256
#define TASK_MINSIZE  4096


struct workerctx_t;
struct mon_task_t;
typedef struct sched_task_t sched_task_t;

/**
 * TASK CONTROL BLOCK
 */
struct lpel_task_t {
  /** intrinsic pointers for organizing tasks in a list*/
  struct lpel_task_t *prev, *next;
  unsigned int uid;    /** unique identifier */
  lpel_taskstate_t state;   /** state */

  struct workerctx_t *worker_context;  /** worker context for this task */

  sched_task_t *sched_info;

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
};




void LpelTaskDestroy( lpel_task_t *t);
void LpelTaskBlockStream( lpel_task_t *ct);
void LpelTaskUnblock( lpel_task_t *ct, lpel_task_t *blocked);

void LpelTaskAddStream( lpel_task_t *t, lpel_stream_desc_t *des, char mode);
void LpelTaskRemoveStream( lpel_task_t *t, lpel_stream_desc_t *des, char mode);

/* local functions
 * defined here to share the implementations for both HRC and DECEN
 */
void TaskStartup( void *data);
void TaskStart( lpel_task_t *t);
void TaskStop( lpel_task_t *t);


#endif /* _TASK_H_ */
