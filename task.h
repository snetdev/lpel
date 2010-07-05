#ifndef _TASK_H_
#define _TASK_H_

#include <pcl.h> /* coroutine_t */

#include "set.h"
#include "timing.h"


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


/*
 * TASK CONTROL BLOCK
 */
struct task {
  /*TODO  type: IO or normal */
  unsigned long uid;
  taskstate_t state;
  task_t *prev, *next;  /* queue handling: prev, next */

  /* signalling events*/
  /*TODO ? padding ? */
  volatile bool *event_ptr;
  volatile bool ev_write, ev_read;

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
  coroutine_t code;
  void *arg;  /* argument */
};




extern task_t *TaskCreate( void (*func)(void *), void *arg, unsigned int attr);
extern void TaskDestroy(task_t *t);
extern void TaskWaitOnRead(void);
extern void TaskWaitOnWrite(void);
extern void TaskExit(void);
extern void TaskYield(void);




#endif /* _TASK_H_ */
