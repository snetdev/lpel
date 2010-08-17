
#include <malloc.h>
#include <assert.h>

#include "task.h"

#include "atomic.h"
#include "timing.h"
#include "lpel.h"

#include "stream.h"
#include "debug.h"


#define BIT_IS_SET(vec,b)   (( (vec) & (b) ) == (b) )

/**
 * 2 to the power of following constant
 * determines the initial group capacity of streamsets
 * in tasks that can wait on any input
 */
#define TASK_WAITANY_GRPS_INIT  2



static sequencer_t taskseq = SEQUENCER_INIT;



/* declaration of startup function */
static void TaskStartup(void *data);


/**
 * Create a task
 */
task_t *TaskCreate( taskfunc_t func, void *inarg, unsigned int attr)
{
  task_t *t = (task_t *)malloc( sizeof(task_t) );
  t->uid = ticket(&taskseq);
  t->state = TASK_INIT;
  t->prev = t->next = NULL;
  t->attr = attr;

  /* initialize reference counter to 1*/
  atomic_set(&t->refcnt, 1);

  t->event_ptr = NULL;
  
  //t->owner = -1;
  t->sched_info = NULL;

  TimingStart(&t->time_alive);
  /* zero all timings */
  TimingZero(&t->time_totalrun);
  t->time_lastrun = t->time_expavg = t->time_totalrun;

  t->cnt_dispatch = 0;

  /* create streamset to write */
  t->streams_write = StreamsetCreate(0);


  /* stuff that is special for WAIT_ANY tasks */
  if (BIT_IS_SET(t->attr, TASK_ATTR_WAITANY)) {
    t->streams_read = StreamsetCreate(TASK_WAITANY_GRPS_INIT);
    rwlock_init( &t->rwlock, LpelNumWorkers() );
    FlagtreeAlloc( &t->flagtree, TASK_WAITANY_GRPS_INIT, &t->rwlock );
    /* max_grp_idx = 2^x - 1 */
    t->max_grp_idx = (1<<TASK_WAITANY_GRPS_INIT)-1;
  } else {
    t->streams_read = StreamsetCreate(0);
  }

  t->code = func;
  /* function, argument (data), stack base address, stacksize */
  t->ctx = co_create(TaskStartup, (void *)t, NULL, TASK_STACKSIZE);
  if (t->ctx == NULL) {
    /*TODO throw error!*/
  }
  t->inarg = inarg;
  t->outarg = NULL;
 
  /* Notify LPEL */
  LpelTaskAdd(t);

  return t;
}


/**
 * Destroy a task
 */
void TaskDestroy(task_t *t)
{
  /* only if no stream points to the flags anymore */
  if ( fetch_and_dec(&t->refcnt) == 1) {
    /* capture end of task lifetime */
    TimingEnd(&t->time_alive);

    /* Notify LPEL first */
    LpelTaskRemove(t);

    /* free the streamsets */
    StreamsetDestroy(t->streams_write);
    StreamsetDestroy(t->streams_read);


    /* waitany-specific cleanup */
    if (BIT_IS_SET(t->attr, TASK_ATTR_WAITANY)) {
      rwlock_cleanup( &t->rwlock );
      FlagtreeFree( &t->flagtree );
    }

    /* delete the coroutine */
    co_delete(t->ctx);

    /* free the TCB itself*/
    free(t);
    t = NULL;
  }
}


/**
 * Hidden Startup function for user specified task function
 *
 * Calls task function with proper signature
 */
static void TaskStartup(void *data)
{
  task_t *t = (task_t *)data;
  taskfunc_t func = t->code;
  /* call the task function with inarg as parameter */
  func(t, t->inarg);

  /* if task function returns, exit properly */
  TaskExit(t, NULL);
}



/**
 * Set Task waiting for a read event
 *
 * @param ct  pointer to the current task
 */
void TaskWaitOnRead(task_t *ct, stream_t *s)
{
  assert( ct->state == TASK_RUNNING );
  
  /* WAIT on read event*/;
  ct->event_ptr = (int *) &s->buf[s->pwrite];
  ct->state = TASK_WAITING;
  ct->wait_on = WAIT_ON_READ;
  
  /* context switch */
  co_resume();
}

/**
 * Set Task waiting for a read event
 *
 * @param ct  pointer to the current task
 */
void TaskWaitOnWrite(task_t *ct, stream_t *s)
{
  assert( ct->state == TASK_RUNNING );

  /* WAIT on write event*/
  ct->event_ptr = (int *) &s->buf[s->pread];
  ct->state = TASK_WAITING;
  ct->wait_on = WAIT_ON_WRITE;
  
  /* context switch */
  co_resume();
}


void TaskWaitAny(task_t *ct, streamtbe_iter_t *iter)
{
  assert( ct->state == TASK_RUNNING );
  assert( BIT_IS_SET(ct->attr, TASK_ATTR_WAITANY) );

  /* WAIT upon any input stream setting root flag */
  ct->event_ptr = &ct->flagtree.buf[0];
  ct->state = TASK_WAITING;
  ct->wait_on = WAIT_ON_ANY;
  
  /* context switch */
  co_resume();

  /* provide iter */
  StreamsetIterateStart(ct->streams_read, iter);
}


/**
 * Exit the current task
 *
 * @param ct  pointer to the current task
 * @param outarg  join argument
 */
void TaskExit(task_t *ct, void *outarg)
{
  assert( ct->state == TASK_RUNNING );

  ct->state = TASK_ZOMBIE;
  
  ct->outarg = outarg;
  /* context switch */
  co_resume();

  /* execution never comes back here */
  assert(0);
}


/**
 * Yield execution back to worker thread
 *
 * @param ct  pointer to the current task
 */
void TaskYield(task_t *ct)
{
  assert( ct->state == TASK_RUNNING );

  ct->state = TASK_READY;
  /* context switch */
  co_resume();
}



/**
 * Join with a task
 */
/*TODO place in special queue which is notified each time a task dies */

