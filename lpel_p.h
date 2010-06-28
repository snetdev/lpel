
#ifndef _LPEL_P_H_
#define _LPEL_P_H_

#include <pcl.h>

#include "timing.h"
#include "lpel.h"

#include "taskqueue.h"
#include "set.h"


#define BUFFER_SIZE 32


/*
 * private LPEL management
 */

extern int LpelGetWorkerId(void);
extern task_t *LpelGetCurrentTask(void);


/*
 * private task management
 */

typedef enum {
  TASK_INIT,
  TASK_RUNNING,
  TASK_READY,
  TASK_WAITING,
  TASK_ZOMBIE
} taskstate_t;

/* TASK CONTROL BLOCK */
struct task {
  /*TODO  type: IO or normal */
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




#endif /* _LPEL_PRIVATE_H_ */
