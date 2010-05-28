
#ifndef _TASK_H_
#define _TASK_H_

typedef struct task task_t;

typedef enum taskstate taskstate_t;

struct task {

  /* TASK CONTROL BLOCK */

  /* type:  IO or normal */
  //TODO

  /* state */
  taskstate_t state;

  /* in case of blocking state: the stream/streamset (queue) holding the task */
  taskqueue_t waitqueue;

  /* queue handling: prev, next */
  task_t *prev;
  task_t *next;

  /* current/last/assigned worker thread */
  // TODO

  /* scheduling information  */
  void *sched_info;

  /* the handle (or NULL for collector?) */
  //TODO


  /* Accounting information */

  /* processing time: */
  //TODO define generic macros and datatypes
  /* time of creation */
  /* last running time */
  /* total running time */
  /* exponential average running time */
  

  /* dispatch counter */
  /* mutex wait counter (sync between worker threads)? */
  /* read counter */
  /* write counter */


  /* CODE */
  /* a coroutine_t variable, upon creation of that task the coroutine needs to be created */
};

enum taskstate {
  TASK_RUNNING,
  TASK_READY,
  TASK_BLOCKED
};



#endif /* _TASK_H */
