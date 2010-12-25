
#include <malloc.h>
#include <assert.h>

#include "arch/atomic.h"
#include "arch/timing.h"

#include "lpel.h"
#include "task.h"

#include "worker.h"
#include "stream.h"
#include "stream_desc.h"

#include "monitoring.h"



static atomic_t taskseq = ATOMIC_INIT(0);



/* declaration of startup function */
static void TaskStartup(void *data);


/**
 * Create a task
 */
task_t *TaskCreate( taskfunc_t func, void *inarg, taskattr_t *attr)
{
  task_t *t = (task_t *)malloc( sizeof(task_t) );

  t->uid = fetch_and_inc( &taskseq);
  t->state = TASK_READY;
  t->prev = t->next = NULL;
  
  /* copy task attributes */
  t->attr = *attr;
  /* fix attributes */
  if (t->attr.stacksize <= 0) {
    t->attr.stacksize = TASK_ATTR_STACKSIZE_DEFAULT;
  }

  /* initialize poll token to 0 */
  atomic_init( &t->poll_token, 0);
  
  t->worker_context = NULL;
  
  if (t->attr.flags & TASK_ATTR_COLLECT_TIMES) {
    TIMESTAMP(&t->times.creat);
  }
  t->cnt_dispatch = 0;

  /* empty dirty list */
  t->dirty_list = STDESC_DIRTY_END;
  

  t->code = func;
  t->inarg = inarg;
  /* function, argument (data), stack base address, stacksize */
  t->ctx = co_create( TaskStartup, (void *)t, NULL, t->attr.stacksize);
  if (t->ctx == NULL) {
    /*TODO throw error!*/
    assert(0);
  }
 
  return t;
}


/**
 * Destroy a task
 */
void TaskDestroy(task_t *t)
{
  atomic_destroy( &t->poll_token);
  /* delete the coroutine */
  co_delete(t->ctx);
  /* free the TCB itself*/
  free(t);
}



/**
 * Call a task (context switch to a task)
 *
 * @pre t->state == TASK_READY
 */
void TaskCall( task_t *t)
{
  assert( t->state == TASK_READY);
  
  t->cnt_dispatch++;
  t->state = TASK_RUNNING;
      
  if (t->attr.flags & TASK_ATTR_COLLECT_TIMES) {
    TIMESTAMP( &t->times.start);
  }

  /*
   * CONTEXT SWITCH
   *
   * CAUTION: A coroutine must not run simultaneously in more than one thread!
   */
  co_call( t->ctx);

  /* task returns in every case in a different state */
  assert( t->state != TASK_RUNNING);

  if (t->attr.flags & (TASK_ATTR_MONITOR_OUTPUT | TASK_ATTR_COLLECT_TIMES)) {
    /* if monitor output, we need a timestamp */
    TIMESTAMP( &t->times.stop);
  }

#ifdef MONITORING_ENABLE
  /* output accounting info */
  if (t->attr.flags & TASK_ATTR_MONITOR_OUTPUT) {
    MonitoringOutput( t->worker_context->mon, t);
  }
#endif
}


/**
 * Block a task
 */
void TaskBlock( task_t *ct, int wait_on)
{
  ct->state = TASK_BLOCKED;
  ct->wait_on = wait_on;
  /* context switch */
  co_resume();
}


/**
 * Exit the current task
 *
 * @param ct  pointer to the current task
 * @pre ct->state == TASK_RUNNING
 */
void TaskExit( task_t *ct)
{
  assert( ct->state == TASK_RUNNING );
  ct->state = TASK_ZOMBIE;
  /* context switch */
  co_resume();
  /* execution never comes back here */
  assert(0);
}


/**
 * Yield execution back to scheduler voluntarily
 *
 * @param ct  pointer to the current task
 * @pre ct->state == TASK_RUNNING
 */
void TaskYield( task_t *ct)
{
  assert( ct->state == TASK_RUNNING );
  ct->state = TASK_READY;
  /* context switch */
  co_resume();
}




/****************************************************************************/
/* Private functions                                                        */
/****************************************************************************/

/**
 * Startup function for user specified task,
 * calls task function with proper signature
 *
 * @param data    the previously allocated task_t TCB
 */
static void TaskStartup(void *data)
{
  task_t *t = (task_t *)data;
  taskfunc_t func = t->code;
  /* call the task function with inarg as parameter */
  func(t, t->inarg);
  /* if task function returns, exit properly */
  TaskExit(t);
}


