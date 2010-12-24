#ifndef _WORKER_H_
#define _WORKER_H_

#include <pthread.h>
#include "bool.h"
#include "scheduler.h"
#include "monitoring.h"
#include "mailbox.h"

struct task;

typedef struct workercfg workercfg_t;

typedef struct workerctx workerctx_t;

struct workercfg {
  int node;
  void (*task_info_print)(void *info);
  bool do_print_workerinfo;
};



struct workerctx {
  int wid; 
  pthread_t     thread;
  unsigned int  num_tasks;
  unsigned int  loop;
  bool          terminate;  
  mailbox_t     mailbox;
  schedctx_t   *sched;
  monitoring_t *mon;
};

void WorkerInit(int size, workercfg_t *cfg);
void WorkerCleanup(void);
void WorkerTerminate(void);


void WorkerTaskWakeup( struct task *by, struct task *whom);
void WorkerTaskAssign( struct task *t, int wid);
void WorkerWrapperCreate( struct task *t, char *name);


#endif /* _WORKER_H_ */
