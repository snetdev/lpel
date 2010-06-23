
#ifndef _LPEL_PRIVATE_H_
#define _LPEL_PRIVATE_H_

#include <pcl.h>

#include "timing.h"
#include "lpel.h"


/*
 * private LPEL management
 */

extern int LpelGetWorkerId(void);
extern task_t *LpelGetCurrentTask(void);


/* private task management */


typedef enum {
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

  /*TODO list of streams opened for writing and reading (as array)*/

  /* CODE */
  coroutine_t code;
  /*TODO ? arg ?*/
  /*TODO the handle (or NULL for collector?) */
};


#endif /* _LPEL_PRIVATE_H_ */
