#ifndef _WORKER_H_
#define _WORKER_H_

#include <pthread.h>
#include "bool.h"
#include "scheduler.h"
#include "monitoring.h"

struct task;

typedef struct workercfg workercfg_t;

typedef struct workerctx workerctx_t;

struct workercfg {
  int node;
  void (*task_info_print)(void *info);
  bool do_print_workerinfo;
};


typedef struct workermsg workermsg_t;


typedef enum {
  WORKER_MSG_TERMINATE,
  WORKER_MSG_WAKEUP,
  WORKER_MSG_ASSIGN,
} workermsg_type_t;

struct workermsg {
  struct workermsg *next;
  workermsg_type_t type;
  struct task *task;
};

struct workerctx {
  int wid; 
  unsigned int     num_tasks;
  unsigned int     loop;
  bool             terminate;  
  pthread_t        thread;
  schedctx_t      *sched;
  monitoring_t    *mon;

  /* messaging */
  pthread_mutex_t  lock_free;
  pthread_mutex_t  lock_inbox;
  pthread_cond_t   notempty;
  workermsg_t     *list_free;
  workermsg_t     *list_inbox;
};

void WorkerInit(int size, workercfg_t *cfg);
void WorkerCleanup(void);
void WorkerTerminate(void);


void WorkerTaskWakeup( struct task *by, struct task *whom);
void WorkerTaskAssign( struct task *t, int wid);
void WorkerWrapperCreate( struct task *t, char *name);


#endif /* _WORKER_H_ */
