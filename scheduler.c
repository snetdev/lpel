/**
 * A simple scheduler
 */

#include <stdlib.h>
#include <pthread.h>
#include <assert.h>

#include "scheduler.h"

#include "taskqueue.h"


struct schedcfg {
  int dummy;
};

struct schedctx {
  int wid;
  taskqueue_t init_queue;
  pthread_mutex_t lock_iq;
  taskqueue_t ready_queue;
  struct {
    taskqueue_t on_read;
    taskqueue_t on_write;
    taskqueue_t on_any;
    int cnt;
  } waiting_queue;
  int cnt_tasks;
};


static int num_workers = -1;
static schedctx_t *schedarray;

/* prototypes for private functions */
static int CollectFromInit(schedctx_t *sc);

static bool WaitingTestOnRead(task_t *wt, void *arg);
static bool WaitingTestOnWrite(task_t *wt, void *arg);
static bool WaitingTestOnAny(task_t *wt, void *arg);


static void WaitingRemove(task_t *wt, void *arg);



/**
 * Initialise scheduler globally
 *
 * Call this function once before any other calls of the scheduler module
 *
 * @param size  size of the worker set, i.e., the total number of workers
 * @param cfg   additional configuration
 */
void SchedInit(int size, schedcfg_t *cfg)
{
  int i;
  schedctx_t *sc;

  assert(0 <= size);

  num_workers = size;
  
  /* allocate the array of scheduler data */
  schedarray = (schedctx_t *) calloc( size, sizeof(schedctx_t) );

  for (i=0; i<size; i++) {
      /* select from the previously allocated schedarray */
      sc = &schedarray[i];
      sc->wid = i;

      TaskqueueInit(&sc->init_queue);
      pthread_mutex_init( &sc->lock_iq, NULL );
      
      TaskqueueInit(&sc->ready_queue);
      
      TaskqueueInit(&sc->waiting_queue.on_read);
      TaskqueueInit(&sc->waiting_queue.on_write);
      TaskqueueInit(&sc->waiting_queue.on_any);
      sc->waiting_queue.cnt = 0;

      sc->cnt_tasks = 0;
  }
}



/**
 * Get scheduler context for a certain worker
 *
 * @pre       0 <= id < size (on SchedInit)
 * @return    pointer to scheduler data
 */
schedctx_t *SchedGetContext(int id)
{
  assert( 0 <= id && id < num_workers );
  return &schedarray[id];
}


/**
 * Cleanup scheduler data
 *
 */
void SchedCleanup(void)
{
  int i;
  schedctx_t *sc;

  for (i=0; i<num_workers; i++) {
    sc = &schedarray[i];
    /* nothing to be done for taskqueue_t */

    /* destroy locks */
    pthread_mutex_destroy(&sc->lock_iq);
  }

  /* free memory for scheduler data */
  free(schedarray);
}



/**
 * Put a task into a ready state, fetchable for execution
 */
void SchedPutReady(schedctx_t *sc, task_t *t)
{
  /* handle depending on the tasks assigned owner */
  if ( t->owner != sc->wid ) {
    /* place into the foreign init queue */
    pthread_mutex_lock( &schedarray[t->owner].lock_iq );
    TaskqueueEnqueue( &schedarray[t->owner].init_queue, t );
    pthread_mutex_unlock( &schedarray[t->owner].lock_iq );
    return;
  }

  /* t is owned by sdt */
  TaskqueueEnqueue( &sc->ready_queue, t );
}


/**
 * Fetch the next task to execute
 */
task_t *SchedFetchNextReady(schedctx_t *sc)
{
  int cnt;

  /* fetch all tasks from init queue and insert into the ready queue */
  cnt = CollectFromInit(sc);
  /* update total number of tasks */
  sc->cnt_tasks += cnt;

  /* wakeup waiting tasks */
  if (sc->waiting_queue.cnt > 0) {
    if (sc->waiting_queue.on_read.count > 0) {
      TaskqueueIterateRemove(
          &sc->waiting_queue.on_read,
          WaitingTestOnRead,  /* on read test */
          WaitingRemove, (void*)sc
          );
    }
    if (sc->waiting_queue.on_write.count > 0) {
      TaskqueueIterateRemove(
          &sc->waiting_queue.on_write,
          WaitingTestOnWrite,  /* on write test */
          WaitingRemove, (void*)sc
          );
    }
    if (sc->waiting_queue.on_any.count > 0) {
      TaskqueueIterateRemove(
          &sc->waiting_queue.on_any,
          WaitingTestOnAny,  /* on any test */
          WaitingRemove, (void*)sc
          );
    }
  }

  /* fetch a task from the ready queue */
  return TaskqueueDequeue( &sc->ready_queue );
}



/**
 * Reschedule a task after execution
 *
 * @param sc   scheduler context
 * @param t    task returned from execution in a different state than READY
 */
void SchedReschedule(schedctx_t *sc, task_t *t)
{
  /* check state of task, place into appropriate queue */
  switch(t->state) {
    case TASK_ZOMBIE:  /* task exited by calling TaskExit() */
      /*TODO handle joinable tasks */
      TaskDestroy(t);
      /* decrement count of managed tasks */
      sc->cnt_tasks--;
      break;

    case TASK_WAITING: /* task returned from a blocking call*/
      {
        taskqueue_t *wq;
        switch (t->wait_on) {
          case WAIT_ON_READ:  wq = &sc->waiting_queue.on_read;  break;
          case WAIT_ON_WRITE: wq = &sc->waiting_queue.on_write; break;
          case WAIT_ON_ANY:   wq = &sc->waiting_queue.on_any;   break;
          default: assert(0);
        }
        /* put into appropriate waiting queue */
        TaskqueueEnqueue( wq, t );
        sc->waiting_queue.cnt++;
      }
      break;

    case TASK_READY: /* task yielded execution  */
      /* put into ready queue */
      TaskqueueEnqueue( &sc->ready_queue, t );
      break;

    default:
      assert(0); /* should not be reached */
  }

}


/**
 * Add a task globally to the scheduler,
 * dependent on the tasks internal state
 *
 * @param t   task to add
 * @return    the scheduler context to which the task was assigned
 */
schedctx_t *SchedAddTaskGlobal(task_t *t)
{
  schedctx_t *sc;
  assert( 0 <= t->owner && t->owner < num_workers);

  sc = &schedarray[t->owner];
  SchedPutReady( sc, t );
  return sc;
}

/** PRIVATE FUNCTIONS *******************************************************/


/**
 * Fetch all tasks from the init queue and enqueue them into the ready queue
 * @param sc    schedctx to operate on
 * @return      number of tasks moved
 */
static int CollectFromInit(schedctx_t *sc)
{
  int cnt;
  task_t *t;

  cnt = 0;


  pthread_mutex_lock( &sc->lock_iq );
  t = TaskqueueDequeue( &sc->init_queue );
  while (t != NULL) {
    //assert( t->state == TASK_INIT );
    assert( t->owner == sc->wid );
    cnt++;
    /* set state to ready and place into the ready queue */
    t->state = TASK_READY;
    TaskqueueEnqueue( &sc->ready_queue, t );
    
    /* for next iteration: */
    t = TaskqueueDequeue( &sc->init_queue );
  }
  pthread_mutex_unlock( &sc->lock_iq );
  return cnt;
}


static bool WaitingTestOnRead(task_t *wt, void *arg)
{
  /* event_ptr points to the position in the buffer
     where task wants to write to */
  return *wt->event_ptr == 0;
}

static bool WaitingTestOnWrite(task_t *wt, void *arg)
{
  /* event_ptr points to the position in the buffer
     where task wants to read from */
  return *wt->event_ptr != 0;
}

static void WaitingTestGather(int i, void *arg)
{
  streamset_t *set = (streamset_t *) arg;
  StreamsetChainAdd( set, i );
}

static bool WaitingTestOnAny(task_t *wt, void *arg)
{
  assert( TASK_IS_WAITANY(wt) );

  /* first of all, check root flag */
  if (*wt->event_ptr != 0) {
    /* if root flag is set, try to gather all set leafs */
    StreamsetChainStart( &wt->streams_read );
    FlagtreeGather(
        &wt->waitany_info->flagtree,
        WaitingTestGather,
        &wt->streams_read
        );
    /* only return true, if at least on leaf could be gathered */
    return StreamsetChainNotEmpty( &wt->streams_read );
  }
  /* event_ptr points to the root of the flagtree */
  return false;
}


static void WaitingRemove(task_t *wt, void *arg)
{
  schedctx_t *sc = (schedctx_t *)arg;

  wt->state  = TASK_READY;
  wt->wait_s = NULL;
  /*wt->event_ptr = NULL;*/
  sc->waiting_queue.cnt--;
  
  /* waiting queue contains only own tasks */
  TaskqueueEnqueue( &sc->ready_queue, wt );
}

