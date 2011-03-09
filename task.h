#ifndef _TASK_H_
#define _TASK_H_

#include <pthread.h>
#include <pcl.h>    /* tasks are executed in user-space with help of
                       GNU Portable Coroutine Library  */

#include "arch/timing.h"
#include "arch/atomic.h"

#include "lpel.h"
#include "worker.h"
#include "scheduler.h"



#define TASK_FLAGS(t, f)  (((t)->flags & (f)) == (f))

/**
 * If a stacksize <= 0 is specified,
 * use the default stacksize
 */
#define LPEL_TASK_ATTR_STACKSIZE_DEFAULT  8192  /* 8k stacksize*/





/* protecting tasks is necessary in case of envisioned work stealing:
 * - a task might decrement a stream semaphore counter and notice it is going to be blocked
 * - before the task does switch context, it can be waken up by a task on another worker
 *   finding the semaphore was <0
 * - as the task is ready, it is subject of theft despite of still being executed
 * - a task must not be executed also by the thief! - the lock physically avoids this
 *
 * Use a pthread mutex for locking, unless following macro is defined:
 */
#define TASK_USE_SPINLOCK

#ifdef TASK_USE_SPINLOCK
#define TASKLOCK_TYPE pthread_spinlock_t
#else
#define TASKLOCK_TYPE pthread_mutex_t
#endif /* TASK_USE_SPINLOCK */

typedef enum {
  TASK_CREATED = 'C',
  TASK_RUNNING = 'U',
  TASK_READY   = 'R',
  TASK_BLOCKED = 'B',
  TASK_ZOMBIE  = 'Z'
} taskstate_t;

typedef enum {
  BLOCKED_ON_INPUT  = 'i',
  BLOCKED_ON_OUTPUT = 'o',
  BLOCKED_ON_ANYIN  = 'a',
} taskstate_blocked_t;


/**
 * TASK CONTROL BLOCK
 */
struct lpel_task_t {
  /** intrinsic pointers for organizing tasks in a list*/
  lpel_task_t *prev, *next;
  unsigned int uid;    /** unique identifier */
  int stacksize;       /** stacksize */
  taskstate_t state;   /** state */
  taskstate_blocked_t blocked_on; /** on which event the task is waiting */

  workerctx_t *worker_context;  /** worker context for this task */

  sched_task_t sched_info;

  /**
   * indicates the SD which points to the stream which has new data
   * and caused this task to be woken up
   */
  lpel_stream_desc_t *wakeup_sd;
  atomic_t poll_token;        /** poll token, accessed concurrently */

  /* ACCOUNTING INFORMATION */
  /* TODO encapsule in mon_task_t mon_info */
  int flags;           /** monitoring flags */
  /** timestamps for creation, start/stop of last dispatch */
  struct {
    timing_t creat, start, stop;
  } times;
  /** dispatch counter */
  unsigned long cnt_dispatch;
  /** streams marked as dirty */
  lpel_stream_desc_t *dirty_list;

  /* CODE */
  TASKLOCK_TYPE lock;
  coroutine_t mctx;     /** context of the task*/
  lpel_taskfunc_t func; /** function of the task */
  void *inarg;          /** input argument  */
};



void _LpelTaskDestroy( lpel_task_t *t);

void _LpelTaskBlock( lpel_task_t *ct, taskstate_blocked_t block_on);
void _LpelTaskUnblock( lpel_task_t *ct, lpel_task_t *blocked);

void _LpelTaskLock( lpel_task_t *t);
void _LpelTaskUnlock( lpel_task_t *t);



#endif /* _TASK_H_ */
