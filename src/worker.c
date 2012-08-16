/**
 * The LPEL worker containing code for workers and wrappers
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <errno.h>

#include <pthread.h>
#include "arch/mctx.h"

#include "arch/atomic.h"

#include "worker.h"
#include "spmdext.h"

#include "task.h"
#include "lpel_main.h"
#include "lpelcfg.h"

#include "mailbox.h"
#include "lpel/monitor.h"

#include "lpel/timing.h"
#include "taskqueue.h"
#include "placementscheduler.h"

#define WORKER_PTR(i) (workers[(i)])




/******************************************************************************/

static int num_workers = -1;
workerctx_t **workers;


#ifdef HAVE___THREAD
static TLSSPEC workerctx_t *workerctx_cur;
#else /* HAVE___THREAD */
static pthread_key_t workerctx_key;
#endif /* HAVE___THREAD */


/* worker thread function declaration */
static void *WorkerThread( void *arg);


static void FetchAllMessages( workerctx_t *wc);




/*******************************************************************************
 *  Convenience functions for sending messages between workers
 ******************************************************************************/

static inline void SendAssign( workerctx_t *target, lpel_task_t *t)
{
  workermsg_t msg;
  /* compose a task assign message */
  msg.type = WORKER_MSG_ASSIGN;
  msg.body.task = t;


  /* send */
  LpelMailboxSend(target->mailbox, &msg);
}



static inline void SendWakeup( workerctx_t *target, lpel_task_t *t)
{
  workermsg_t msg;
  /* compose a task wakeup message */
  msg.type = WORKER_MSG_WAKEUP;
  msg.body.task = t;
  /* send */
  LpelMailboxSend(target->mailbox, &msg);
}



/**
 * Get current worker context
 */
static inline workerctx_t *GetCurrentWorker(void)
{
#ifdef HAVE___THREAD
  return workerctx_cur;
#else /* HAVE___THREAD */
  return (workerctx_t *) pthread_getspecific(workerctx_key);
#endif /* HAVE___THREAD */
}


/******************************************************************************/

/**
 * Initialise worker globally
 *
 *
 * @param size    size of the worker set, i.e., the total number of workers
 */
void LpelWorkerInit(int size)
{
  int i, res;

  assert(0 <= size);
  num_workers = size;


#ifndef HAVE___THREAD
  /* init key for thread specific data */
  pthread_key_create(&workerctx_key, NULL);
#endif /* HAVE___THREAD */

  /* initialize spmdext module */
  res = LpelSpmdInit(num_workers);

  /* allocate worker context table */
  workers = (workerctx_t **) malloc( num_workers * sizeof(workerctx_t*) );
  /* allocate worker contexts and prepare data structures */
  for( i=0; i<num_workers; i++) {
    workerctx_t *wc = workers[i] = malloc(sizeof(workerctx_t));
    wc->wid = i;
    wc->num_tasks = 0;
    wc->terminate = 0;
#ifdef MEASUREMENTS
    wc->max_time = 0;
    wc->min_time = 0;
    wc->total_tasks = 0;
#endif

    wc->sched = LpelSchedCreate( i);
    wc->wraptask = NULL;

#ifdef USE_LOGGING

    if (MON_CB(worker_create)) {
      wc->mon = MON_CB(worker_create)(wc->wid);
    } else {
      wc->mon = NULL;
    }
#else
    wc->mon = NULL;
#endif

    /* mailbox */
    wc->mailbox = LpelMailboxCreate();

    /* LIFO of free tasks */
    atomic_init( &wc->free_tasks, NULL);

    wc->placement_data = PlacementInitWorker();
  }

  /* Initialize placement scheduler */
  LpelPlacementSchedulerInit();

  assert(res==0);
}


/**
 * Cleanup worker contexts
 *
 */
void LpelWorkerCleanup(void)
{
#ifdef MEASUREMENTS
  long min_time = 0, max_time = 0, total_tasks = 0;
#endif
  workerctx_t *wc;

  /* wait on workers */
  for(int i = 0; i < num_workers; i++) {
    wc = WORKER_PTR(i);
    /* wait for the worker to finish */
    (void) pthread_join( wc->thread, NULL);
  }
  /* cleanup the data structures */
  for(int i = 0; i < num_workers; i++) {
    wc = WORKER_PTR(i);

#ifdef MEASUREMENTS
    min_time += wc->min_time;
    max_time += wc->max_time;
    total_tasks += wc->total_tasks;
#endif

    LpelMailboxDestroy(wc->mailbox);
    LpelSchedDestroy( wc->sched);
    PlacementDestroyWorker(wc->placement_data);
    free(wc);
  }

  /* free workers table */
  free( workers);

  /* free mutex table */

#ifdef MEASUREMENTS
  min_time /= num_workers;
  max_time /= num_workers;

  printf("WORKER STATISTICS:\n");
  printf("MINIMUM TIME:\t%lf\n", (double)min_time / (double)1000000000);
  printf("MAXIMUM TIME:\t%lf\n", (double)max_time / (double)1000000000);
  printf("TOTAL NUMBER OF TASKS:\t%ld\n", total_tasks);
#endif

  /* cleanup spmdext module */
  LpelSpmdCleanup();

#ifndef HAVE___THREAD
  pthread_key_delete(workerctx_key);
#endif /* HAVE___THREAD */
}





/**
 * Assign a task to the worker by sending an assign message to that worker
 */
void LpelWorkerRunTask( lpel_task_t *t)
{
  assert( t->state == TASK_CREATED );
  SendAssign( t->worker_context, t);
}



workerctx_t *LpelWorkerSelf(void)
{
  return GetCurrentWorker();
}


lpel_task_t *LpelWorkerCurrentTask(void)
{
  workerctx_t *w = GetCurrentWorker();
  /* It is quite a common bug to call LpelWorkerCurrentTask() from a non-task context.
   * Provide an assertion error instead of just segfaulting on a null dereference. */
  assert(w && "Currently not in an LPEL worker context!");
  return w->current_task; 
}


/**
 * Dispatch next ready task
 *
 * This dispatcher function is called upon blocking a task.
 * It executes on the same exec-stack as the task itself, calls the FetchReady
 * function from the scheduler module, and, if there is a ready task, makes
 * a continuation to that ready task. If no task is ready, execution returns to the
 * worker loop (wc->mctx). If the task runs on a wrapper, execution returns to the
 * wrapper loop in either case.
 * This way, in the optimal case, task switching only requires a single context switch
 * instead of two.
 *
 * @param t   the current task context
 */
void LpelWorkerDispatcher( lpel_task_t *t)
{
  workerctx_t *wc = t->worker_context;

  /* dependent of worker or wrapper */
  if (wc->wid != -1) {
    lpel_task_t *next;

    /* before picking the next task, process messages to consider
     * also newly arrived READY tasks
     */
    FetchAllMessages( wc);

    /* before executing a task, handle all pending requests! */
    LpelSpmdHandleRequests(wc->wid);

    next = LpelSchedFetchReady( wc->sched);
    if (next != NULL) {
      /* short circuit */
      if (next==t) { return; }

      /* execute task */
      wc->current_task = next;
      mctx_switch(&t->mctx, &next->mctx); /*SWITCH*/
    } else {
      /* no ready task! -> back to worker context */
      wc->current_task = NULL;
      mctx_switch(&t->mctx, &wc->mctx); /*SWITCH*/
    }
  } else {
    /* we are on a wrapper.
     * back to wrapper context
     */
    wc->current_task = NULL;
    mctx_switch(&t->mctx, &wc->mctx); /*SWITCH*/
    /* nothing to finalize on a wrapper */
  }
  /*********************************
   * ... CTX SWITCH ...
   *********************************/
  /* let task continue its business ... */
}



void LpelWorkerSpawn(void)
{
  int i;
  /* create worker threads */
  for( i=0; i<num_workers; i++) {
    workerctx_t *wc = WORKER_PTR(i);
    /* spawn joinable thread */
    (void) pthread_create( &wc->thread, NULL, WorkerThread, wc);
  }
}




/**
 * Wakeup a task from within another task - this internal function
 * is called in _LpelTaskUnblock()
 * TODO: wakeup from without a task, i.e. from kernel by an asynchronous
 *       interrupt for a completed request?
 *       -> by == NULL, at least there is no valid by->worker_context
 */
void LpelWorkerTaskWakeup( lpel_task_t *by, lpel_task_t *whom)
{
  /* worker context of the task to be woken up */
  workerctx_t *wc = whom->worker_context;

  if (wc->wid < 0) {
    SendWakeup( wc, whom);
  } else {
    if ( !by || (by->worker_context != whom->worker_context)) {
      SendWakeup( wc, whom);
    } else {
      assert(whom->state != TASK_READY);
      whom->state = TASK_READY;
      LpelSchedMakeReady( wc->sched, whom);
    }
  }
}

void LpelWorkerTaskWakeupLocal( workerctx_t *wc, lpel_task_t *task)
{
  assert(task->state != TASK_READY);
  assert(task->worker_context == wc);
  task->state = TASK_READY;
  LpelSchedMakeReady( wc->sched, task);
}


void LpelWorkerBroadcast(workermsg_t *msg)
{
  int i;
  workerctx_t *wc;

  for( i=0; i<num_workers; i++) {
    wc = WORKER_PTR(i);
    /* send */
    LpelMailboxSend(wc->mailbox, msg);
  }
}

/**
 * Broadcast a termination message
 */
void LpelWorkerTerminate(void)
{
  workermsg_t msg;

  /* compose a task term message */
  msg.type = WORKER_MSG_TERMINATE;
  LpelWorkerBroadcast(&msg);
}



/**
 * Get a worker context from the worker id
 */
workerctx_t *LpelWorkerGetContext(int id)
{

  workerctx_t *wc = NULL;

  if (id >= 0 && id < num_workers) {
    wc = WORKER_PTR(id);
  }

  /* create a new worker context for a wrapper */
  if (id == -1) {
    wc = (workerctx_t *) malloc( sizeof( workerctx_t));
    wc->wid = -1;
    wc->terminate = 0;
    /* Wrapper is excluded from scheduling module */
    wc->sched = NULL;
    wc->wraptask = NULL;
    wc->mon = NULL;
    /* mailbox */
    wc->mailbox = LpelMailboxCreate();
    /* LIFO of free tasks */
    atomic_init( &wc->free_tasks, NULL);

    (void) pthread_create( &wc->thread, NULL, WorkerThread, wc);
    (void) pthread_detach( wc->thread);
  }

  assert((wc != NULL) && "The worker of the requested id does not exist.");
  return wc;
}



void LpelWorkerSelfTaskYield(lpel_task_t *t)
{
  workerctx_t *wc = t->worker_context;

  if (wc->wid < 0) {
    wc->wraptask = t;
  } else {
    LpelSchedMakeReady( wc->sched, t);
  }
}

/******************************************************************************/
/*  PRIVATE FUNCTIONS                                                         */
/******************************************************************************/

static void ProcessMessage( workerctx_t *wc, workermsg_t *msg)
{
  lpel_task_t *t;

  //WORKER_DBGMSG(wc, "worker %d processing msg %d\n", wc->wid, msg->type);

  switch( msg->type) {
    case WORKER_MSG_WAKEUP:
      /* worker has new ready tasks,
       * just wakeup to continue loop
       */
      t = msg->body.task;
      assert(t->state != TASK_READY);
      t->state = TASK_READY;

      WORKER_DBGMSG(wc, "Received wakeup for %d.\n", t->uid);

      if (wc->wid < 0) {
        wc->wraptask = t;
      } else {
        LpelSchedMakeReady( wc->sched, t);
      }
      break;

    case WORKER_MSG_TERMINATE:
      wc->terminate = 1;
      break;

    case WORKER_MSG_ASSIGN:
      t = msg->body.task;

      assert(t->state == TASK_CREATED);
      t->state = TASK_READY;

      wc->num_tasks++;
      WORKER_DBGMSG(wc, "Assigned task %d.\n", t->uid);

      if (wc->wid < 0) {
        wc->wraptask = t;
        /* create monitoring context if necessary */
#ifdef USE_LOGGING
        if (t->mon) {
          if (MON_CB(worker_create_wrapper)) {
            wc->mon = MON_CB(worker_create_wrapper)(t->mon);
          } else {
            wc->mon = NULL;
          }
          if ( wc->mon && MON_CB(worker_waitstart)) {
            MON_CB(worker_waitstart)(wc->mon);
          }
        }
#endif
      } else {
        LpelSchedMakeReady( wc->sched, t);
#ifdef MEASUREMENTS
        LpelTimingEnd(&t->start_time);
        long time = LpelTimingToNSec(&t->start_time);
        wc->max_time = (time > wc->max_time) ? time : wc->max_time;
        wc->min_time = (time < wc->min_time || wc->min_time == 0) ? time : wc->min_time;
        wc->total_tasks++;
#endif
      }

#ifdef USE_LOGGING
      /* assign monitoring context to taskmon */
      if (t->mon && MON_CB(task_assign)) {
        MON_CB(task_assign)(t->mon, wc->mon);
      }
#endif
      break;

    case WORKER_MSG_SPMDREQ:
      assert(wc->wid >= 0);
      /* This message serves the sole purpose to wake up any sleeping workers,
       * as handling of requests is done before execution of a task.
       */
      /*
      WORKER_DBGMSG(wc, "Received spmd request notification"
            " from worker %d!\n", msg->body.from_worker);
      */
      break;

    default: assert(0);
  }
}


static void WaitForNewMessage( workerctx_t *wc)
{
  workermsg_t msg;

#ifdef USE_LOGGING
  if (wc->mon && MON_CB(worker_waitstart)) {
    MON_CB(worker_waitstart)(wc->mon);
  }
#endif

  LpelMailboxRecv(wc->mailbox, &msg);

#ifdef USE_LOGGING
  if (wc->mon && MON_CB(worker_waitstop)) {
    MON_CB(worker_waitstop)(wc->mon);
  }
#endif

  ProcessMessage( wc, &msg);
}


static void FetchAllMessages( workerctx_t *wc)
{
  workermsg_t msg;
  while( LpelMailboxHasIncoming(wc->mailbox) ) {
    LpelMailboxRecv(wc->mailbox, &msg);
    ProcessMessage( wc, &msg);
  }
}


/**
 * Worker loop
 */
static void WorkerLoop( workerctx_t *wc)
{
  lpel_task_t *t = NULL;

  do {
    /* before executing a task, handle all pending requests! */
    LpelSpmdHandleRequests(wc->wid);

    t = LpelSchedFetchReady( wc->sched);
    if (t != NULL) {
      /* execute task */
      wc->current_task = t;
      mctx_switch(&wc->mctx, &t->mctx);

      WORKER_DBGMSG(wc, "Back on worker %d context.\n", wc->wid);

      /* check if there are any contexts marked for deletion. */
      LpelCollectTask(wc, NULL);

    } else {
      /* no ready tasks */
      WaitForNewMessage( wc);
    }
    /* fetch (remaining) messages */
    FetchAllMessages( wc);
  } while ( !( 0==wc->num_tasks && wc->terminate) );

}


static void WrapperLoop( workerctx_t *wc)
{
  lpel_task_t *t = NULL;

  do {
    t = wc->wraptask;
    if (t != NULL) {
      /* execute task */
      wc->current_task = t;
      wc->wraptask = NULL;
      mctx_switch(&wc->mctx, &t->mctx);

    } else {
      /* no ready tasks */
      WaitForNewMessage( wc);
    }
    /* fetch (remaining) messages */
    FetchAllMessages( wc);
  } while ( !wc->terminate);
}


/**
 * Thread function for workers (and wrappers)
 */
static void *WorkerThread( void *arg)
{
  workerctx_t *wc = (workerctx_t *)arg;
  lpel_task_t *t;

#ifdef HAVE___THREAD
  workerctx_cur = wc;
#else /* HAVE___THREAD */
  /* set pointer to worker context as TSD */
  pthread_setspecific(workerctx_key, wc);
#endif /* HAVE___THREAD */


//FIXME
#ifdef USE_MCTX_PCL
  assert( 0 == co_thread_init());
  wc->mctx = co_current();
#endif


  wc->current_task = NULL;

  /* no task marked for deletion */
  wc->marked_del = NULL;

  /* assign to cores */
  LpelThreadAssign( wc->wid);

  /*******************************************************/
  if ( wc->wid >= 0) {
    WorkerLoop( wc);
  } else {
    WrapperLoop( wc);
  }
  /*******************************************************/

#ifdef USE_LOGGING
  /* cleanup monitoring */
  if (wc->mon && MON_CB(worker_destroy)) {
    MON_CB(worker_destroy)(wc->mon);
  }
#endif

  /* destroy all the free tasks */
  while ((t = LpelPopTask(free, &wc->free_tasks))) {
      LpelTaskDestroy(t);
  }

  /* on a wrapper, we also can cleanup more*/
  if (wc->wid < 0) {
    /* clean up the mailbox for the worker */
    LpelMailboxDestroy(wc->mailbox);

    /* free the worker context */
    free( wc);
  }

#ifdef USE_MCTX_PCL
  co_thread_cleanup();
#endif

  return NULL;
}

/** return the total number of workers */
int LpelWorkerCount(void)
{
  return num_workers;
}
