#ifndef _TASK_H_
#define _TASK_H_

#include <pcl.h> /* coroutine_t */

#include "set.h"
#include "timing.h"
#include "atomic.h"


#define TASK_STACKSIZE  8192  /* 8k stacksize*/


/* 64bytes is the common size of a cache line */
#define longxCacheLine  (64/sizeof(long))

typedef enum {
  TASK_TYPE_NORMAL,
  TASK_TYPE_IO
} tasktype_t;


typedef enum {
  TASK_INIT,
  TASK_RUNNING,
  TASK_READY,
  TASK_WAITING,
  TASK_ZOMBIE
} taskstate_t;


typedef struct task task_t;

typedef void (*taskfunc_t)(task_t *t, void *inarg);

/*
 * TASK CONTROL BLOCK
 */
struct task {
  /* put the signalling flags on front (alignment) */
  volatile unsigned long ev_read;
  long padding1[longxCacheLine-1];
  volatile unsigned long ev_write;
  long padding2[longxCacheLine-1];

  /*TODO  type: IO or normal */
  unsigned long uid;
  taskstate_t state;
  task_t *prev, *next;  /* queue handling: prev, next */

  /* pointer to signalling flag */
  volatile unsigned long *event_ptr;

  /* reference counter */
  atomic_t refcnt;

  int owner;         /* owning worker thread TODO as place_t */
  void *sched_info;  /* scheduling information  */

  /* Accounting information */
  /* processing time: */
  timing_t time_created;  /*XXX time of creation */
  timing_t time_exited;   /*XXX time of exiting */
  timing_t time_alive;    /* time alive */
  timing_t time_lastrun;  /* last running time */
  timing_t time_totalrun; /* total running time */
  timing_t time_expavg;   /* exponential average running time */
  unsigned long cnt_dispatch; /* dispatch counter */

  /* array of streams opened for writing/reading */
  set_t streams_writing, streams_reading;

  /* CODE */
  coroutine_t ctx;
  taskfunc_t code;
  void *inarg;  /* input argument  */
  void *outarg; /* output argument */
};




extern task_t *TaskCreate( taskfunc_t, void *inarg, unsigned int attr);
extern void TaskDestroy(task_t *t);
extern void TaskWaitOnRead(task_t *ct);
extern void TaskWaitOnWrite(task_t *ct);
extern void TaskExit(task_t *ct, void *outarg);
extern void TaskYield(task_t *ct);




#endif /* _TASK_H_ */
