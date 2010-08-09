/**
 * A simple scheduler
 */

#include <stdlib.h>

#include "scheduler.h"

#include "taskqueue.h"
#include "mpscqueue.h"

typedef struct scheddata scheddata_t;

struct scheddata {
  int id;
  taskqueue_t ready_queue;
  taskqueue_t waiting_queue;
  mpscqueue_t init_queue;
  int cnt_tasks;
};


static int num_workers = -1;
static int num_current = 0;
static scheddata_t schedarray[];

/* prototypes for private functions */
static bool WaitingTest(task_t *wt, void *arg);
static void WaitingRemove(task_t *wt, void *arg);



/**
 * Initialise scheduler data for a specific worker
 *
 * Calls to SchedInit must not take place concurrently!
 *
 * @param id    unique id of worker to initialise the data for
 * @param size  size of the worker set, i.e., the total number of workers
 * @pre         successive calls of SchedInit have to specify
                different ids but the same size
 * @pre         0 <= id < size
 * @return      pointer to scheduler data
 */
scheddata_t *SchedInit(int id, int size)
{
  scheddata_t *sdt;

  assert(0 <= id && id < size);

  if (num_workers == -1) {
    assert(num_current==0);
    num_workers = size;
    /* allocate the array of scheduler data */
    schedarray = (scheddata_t *) calloc( num_workers, sizeof(scheddata_t) );
  } else {
    assert( num_workers == size );
  }

  num_current++;

  /* select from the previously allocated schedarray */
  sdt = schedarray[id];
  sdt->id = id;
  TaskqueueInit(&sdt->ready_queue);
  TaskqueueInit(&sdt->waiting_queue);
  sdt->initqueue = MPSCQueueCreate();
  sdt->cnt_tasks = 0;
  return sdt;
}




/**
 * Destroy scheduler data
 *
 * Calls to SchedDestroy must not take place concurrently!
 *
 * @param sdt   scheduler data to destroy
 */
void SchedDestroy(scheddata_t *sdt)
{ 
  /* nothing to be done for taskqueue_t */

  /* destroy initqueue */
  MPSCQueueDestroy(sdt->initqueue);

  /* free memory for scheduler data */
  free(sdt);
  
  num_current--;
  if (0 == num_current) free(schedarray);
}



/**
 * Put a task into a ready state, fetchable for execution
 */
void SchedPutReady(scheddata_t *sdt, task_t *t)
{
  /* handle depending on the tasks assigned owner */
  if ( t->owner != sdt->id ) {
    /* place into the foreign init queue */
    MPSCQueueEnqueue( schedarray[t->owner].init_queue, (void *) t );
    return;
  }

  /* t is owned by sdt */
  TaskqueueAppend( &sdt->ready_queue, t );
}


/**
 * Fetch the next task to execute
 */
task_t *SchedFetchNextReady(scheddata_t *sdt)
{
  int cnt;

  /* fetch all tasks from init queue and insert into the ready queue */
  cnt = CollectFromInit(sdt);
  /* update total number of tasks */
  sdt->cnt_tasks += cnt;

  /* wakeup waiting tasks */
  if (sdt->queue_waiting.count > 0) {
    /* iterate through waiting queue, check r/w events */
    TaskqueueIterateRemove( &sdt->queue_waiting,
        WaitingTest, WaitingRemove, (void*)sdt
        );
  }

  /* fetch a task from the ready queue */
  return TaskqueueRemove( &sdt->ready_queue );
}



/**
 * Reschedule a task after execution
 *
 * @param sdt   scheduler data
 * @param t     task returned from execution in a different state than READY
 */
void SchedReschedule(scheddata_t *sdt, task_t *t)
{
  /* check state of task, place into appropriate queue */
  switch(t->state) {
    case TASK_ZOMBIE:  /* task exited by calling TaskExit() */
      /*TODO handle joinable tasks */
      TaskDestroy(t);
      /* decrement count of managed tasks */
      sdt->cnt--;
      break;

    case TASK_WAITING: /* task returned from a blocking call*/
      /* put into waiting queue */
      TaskqueueAppend( &sdt->waiting_queue, t );
      break;

    case TASK_READY: /* task yielded execution  */
      /* put into ready queue */
      TaskqueueAppend( &sdt->ready_queue, t );
      break;

    default:
      assert(0); /* should not be reached */
  }

}


/** PRIVATE FUNCTIONS *******************************************************/


/**
 * Fetch all tasks from the init queue and enqueue them into the ready queue
 * @param sdt   scheddata to operate on
 * @return      number of tasks moved
 */
static int CollectFromInit(scheddata_t *sdt)
{
  int cnt;
  task_t *t;

  cnt = 0;
  t = (task_t *) MPSCQueueDequeue( sdt->queue_init );
  while (t != NULL) {
    //assert( t->state == TASK_INIT );
    assert( t->owner == sdt->id );
    cnt++;
    /* set state to ready and place into the ready queue */
    t->state = TASK_READY;
    TaskqueueAppend( &sdt->ready_queue, t );
    
    /* for next iteration: */
    t = (task_t *) MPSCQueueDequeue( sdt->queue_init );
  }
  return cnt;
}


static bool WaitingTest(task_t *wt, void *arg)
{
  return *wt->event_ptr != 0;
}

static void WaitingRemove(task_t *wt, void *arg)
{
  scheddata_t *sdt = (scheddata_t *)arg;

  wt->state = TASK_READY;
  *wt->event_ptr = 0;
  wt->event_ptr = NULL;
  
  /* waiting queue contains only own tasks */
  TaskqueueAppend( &sdt->ready_queue, wt );
}

