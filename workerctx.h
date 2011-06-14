#ifndef _WORKERCTX_H_
#define _WORKERCTX_H_

/* includes for the datatypes defined in workerctx_t struct */

#include <pthread.h>
#include <pcl.h>
#include "monitoring.h"
#include "task.h"
#include "scheduler.h"
#include "mailbox.h"



typedef struct workerctx_t {
  int wid;
  pthread_t     thread;
  coroutine_t   mctx;
  int           terminate;
  unsigned int  num_tasks;
  //taskqueue_t   free_tasks;
  lpel_task_t  *current_task;
  lpel_task_t  *marked_del;
  monctx_t     *mon;
  mailbox_t    *mailbox;
  schedctx_t   *sched;
  lpel_task_t  *wraptask;
  char          padding[64];
} workerctx_t;



#endif /* _WORKERCTX_H_ */
