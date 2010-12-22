#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include <pthread.h>
#include "lpel.h"
#include "taskqueue.h"
#include "monitoring.h"

struct task;

typedef struct schedcfg schedcfg_t;

typedef struct schedctx schedctx_t;

struct schedcfg {
  int node;
  void (*task_info_print)(void *info);
  bool do_print_schedinfo;
};



struct schedctx {
  int wid; 
  unsigned int     num_tasks;
  unsigned int     loop;
  taskqueue_t      queue;
  pthread_mutex_t  lock;
  pthread_cond_t   cond;
  bool             terminate;  
  pthread_t        thread;
  monitoring_t    *mon;
};

void SchedInit(int size, schedcfg_t *cfg);
void SchedCleanup(void);
void SchedTerminate(void);

struct lpelthread;

void SchedAssignTask( struct task *t, int wid);
void SchedWrapper( struct task *t, char *name);
void SchedWakeup( struct task *by, struct task *whom);


#endif /* _SCHEDULER_H_ */
