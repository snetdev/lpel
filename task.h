#ifndef _TASK_H_
#define _TASK_H_

#include <pcl.h> /* coroutine_t */

#include "streamset.h"
#include "flagtree.h"
#include "rwlock.h"
#include "timing.h"
#include "atomic.h"


#define TASK_STACKSIZE  8192  /* 8k stacksize*/


/* 64bytes is the common size of a cache line */
#define longxCacheLine  (64/sizeof(long))

#define TASK_ATTR_DEFAULT      (0)
#define TASK_ATTR_WAITANY   (1<<0)


#define BIT_IS_SET(vec,b)   (( (vec) & (b) ) == (b) )

/**
 * Check if a task is a waitany-task
 * @param t   pointer to task_t
 */
#define TASK_IS_WAITANY(t)  (BIT_IS_SET((t)->attr, TASK_ATTR_WAITANY))


struct stream;


typedef enum {
  TASK_INIT    = 'I',
  TASK_RUNNING = 'U',
  TASK_READY   = 'R',
  TASK_WAITING = 'W',
  TASK_ZOMBIE  = 'Z'
} taskstate_t;

typedef enum {
  WAIT_ON_READ  = 'r',
  WAIT_ON_WRITE = 'w',
  WAIT_ON_ANY   = 'a'
} taskstate_wait_t;


struct waitany {
  flagtree_t flagtree;
  rwlock_t rwlock;
  int max_grp_idx;
  streamtbe_iter_t iter;
};

typedef struct task task_t;

typedef void (*taskfunc_t)(task_t *t, void *inarg);

/*
 * TASK CONTROL BLOCK
 */
struct task {
  unsigned long uid;
  taskstate_t state;
  task_t *prev, *next;  /* queue handling: prev, next */

  /* attributes */
  unsigned int attr;

  /* pointer to signalling flag */
  volatile int *event_ptr;
  taskstate_wait_t wait_on;
  struct stream *wait_s;

  /* waitany-task specific stuff */
  struct waitany *waitany_info;

  /* reference counter */
  atomic_t refcnt;

  int owner;         /* owning worker thread TODO as place_t */
  void *sched_info;  /* scheduling information  */

  /* ACCOUNTING INFORMATION */
  /* timestamps for creation, start/stop of last dispatch */
  struct {
    timing_t creat, start, stop;
  } times;
  /* dispatch counter */
  unsigned long cnt_dispatch;
  /* streamtables: streams opened for writing/reading */
  streamset_t streams_read, streams_write;

  /* CODE */
  coroutine_t ctx;
  taskfunc_t code;
  void *inarg;  /* input argument  */
  void *outarg; /* output argument */
};


struct stream;

extern task_t *TaskCreate( taskfunc_t, void *inarg, unsigned int attr);
extern void TaskDestroy(task_t *t);
extern void TaskWaitOnRead(task_t *ct, struct stream *s);
extern void TaskWaitOnWrite(task_t *ct, struct stream *s);
extern void TaskWaitOnAny(task_t *ct);
extern void TaskExit(task_t *ct, void *outarg);
extern void TaskYield(task_t *ct);




#endif /* _TASK_H_ */
