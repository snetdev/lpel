#ifndef _TASK_H_
#define _TASK_H_


#include <lpel.h>
#include <pthread.h>

#include "arch/mctx.h"
#include "placementscheduler.h"


#include "arch/atomic.h"

#include "scheduler.h"


/**
 * If a task size <= 0 is specified,
 * use the default size
 */
#define LPEL_TASK_SIZE_DEFAULT  8192  /* 8k */

struct workerctx_t;
struct mon_task_t;


/**
 * TASK CONTROL BLOCK
 */
struct lpel_task_t {
  /** intrinsic pointers for organizing tasks in a list*/
  struct lpel_task_t *prev, *next, *free, *migrate;
  unsigned int uid;    /** unique identifier */
  enum lpel_taskstate_t state;   /** state */

  struct workerctx_t *worker_context;  /** worker context for this task */

  sched_task_t sched_info;

  /**
   * indicates the SD which points to the stream which has new data
   * and caused this task to be woken up
   */
  struct lpel_stream_desc_t *wakeup_sd;
  atomic_int poll_token;        /** poll token, accessed concurrently */

  /* ACCOUNTING INFORMATION */
  struct mon_task_t *mon;

  /* CODE */
  mctx_t mctx;          /** machine context of the task*/
  void *stack;          /** allocated stack */
  lpel_taskfunc_t func; /** function of the task */
  void *inarg;          /** input argument  */
  int terminate;
  int size;             /** stack size */

  int current_worker;   /** The current worker id */

  /**
   * Indicates the worker id which the placement scheduler has decided
   * the task should migrate to, if it is the same as current worker,
   * it keeps running on the current worker.
   */
  int new_worker;

  task_placement_t *placement_data;

  /* user data */
  void *usrdata;
  const char *name;
  /* destructor for user data */
  lpel_usrdata_destructor_t usrdt_destr;
  void (*name_destr)(void *);
};




void LpelTaskDestroy( lpel_task_t *t);


void LpelTaskBlock( lpel_task_t *t );
void LpelTaskBlockStream( lpel_task_t *ct);
void LpelTaskUnblock( lpel_task_t *ct, lpel_task_t *blocked);
#endif /* _TASK_H_ */
