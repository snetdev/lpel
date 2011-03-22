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
#include <pcl.h>

#include "arch/atomic.h"

#include "worker.h"

#include "task.h"
#include "threading.h"
#include "monitoring.h"


struct workerctx_t {
  int wid; 
  pthread_t     thread;
  coroutine_t   mctx;
  lpel_task_t  *predecessor;
  int           terminate;
  timing_t      wait_time;
  unsigned int  wait_cnt;
  unsigned int  num_tasks;
  //taskqueue_t   free_tasks;
  monctx_t     *mon;
  mailbox_t     mailbox;
  schedctx_t   *sched;
  lpel_task_t  *wraptask;
  char          padding[64];
};


static int num_workers = -1;
static workerctx_t *workers;
static workercfg_t  config;



/* worker thread function declaration */
static void *WorkerThread( void *arg);


static void RescheduleTask( workerctx_t *wc, lpel_task_t *t);
static void FetchAllMessages( workerctx_t *wc);



/******************************************************************************/
/*  Convenience functions for sending messages between workers                */
/******************************************************************************/


static inline void SendAssign( workerctx_t *target, lpel_task_t *t)
{
  workermsg_t msg;
  /* compose a task assign message */
  msg.type = WORKER_MSG_ASSIGN;
  msg.body.task = t;


  /* send */
  MailboxSend( &target->mailbox, &msg);
}



static inline void SendWakeup( workerctx_t *target, lpel_task_t *t)
{
  workermsg_t msg;
  /* compose a task wakeup message */
  msg.type = WORKER_MSG_WAKEUP;
  msg.body.task = t;
  /* send */
  MailboxSend( &target->mailbox, &msg);
}


/******************************************************************************/
/*  PUBLIC FUNCTIONS                                                          */
/******************************************************************************/

/**
 * Assign a task to the worker by sending an assign message to that task
 */
void LpelWorkerRunTask( lpel_task_t *t)
{
  assert( t->state == TASK_CREATED );
  SendAssign( t->worker_context, t);
}



/**
 * Broadcast a termination message
 */
void LpelWorkerTerminate(void)
{
  int i;
  workerctx_t *wc;
  workermsg_t msg;

  /* compose a task term message */
  msg.type = WORKER_MSG_TERMINATE;

  /* broadcast a terminate message */
  for( i=0; i<num_workers; i++) {
    wc = &workers[i];

    /* send */
    MailboxSend( &wc->mailbox, &msg);
  }
}


/******************************************************************************/
/*  HIDDEN FUNCTIONS                                                          */
/******************************************************************************/




void LpelWorkerFinalizeTask( workerctx_t *wc)
{
  if (wc->predecessor != NULL) {
    /**
     * release the task wc->predecessor
     */
    RescheduleTask( wc, wc->predecessor);
    wc->predecessor = NULL;
  }
}


static inline void LpelWorkerTaskCall( workerctx_t *wc, lpel_task_t *t)
{
  /**
   * aquire the task t
   */
  co_call( t->mctx);
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

  /* set this task as predecessor on worker, for finalization*/
  wc->predecessor = t;

  /* dependent of worker or wrapper */
  if (wc->wid != -1) {
    lpel_task_t *next;

    /* FIXME? following could result setting t->state to ready again,
     * resulting in wrong behaviour at the next FinalizeTask(), i.e.,
     * double entry in ready queue
     */
    // FetchAllMessages( wc);
    
    next = SchedFetchReady( wc->sched);
    if (next != NULL) {
      /* short circuit */
      //if (next==t) { return; }
      assert(next != t);

      /* execute task */
      LpelWorkerTaskCall( wc, next);
    } else {
      /* no ready task! -> back to worker context */
      co_call( wc->mctx);
    }
    /*********************************
     * ... CTX SWITCH ...
     *********************************/

    /* upon return, finalize previously executed task */
    LpelWorkerFinalizeTask( wc);

    /* process all incoming messages */
    FetchAllMessages( wc);

  } else {
    /* we are on a wrapper.
     * back to wrapper context
     */
    co_call( wc->mctx);
    /* nothing to finalize on a wrapper */
  }
  /* let task continue its business ... */
}





/**
 * Initialise worker globally
 *
 *
 * @param size  size of the worker set, i.e., the total number of workers
 * @param cfg   additional configuration
 */
void LpelWorkerInit(int size, workercfg_t *cfg)
{
  int i;

  assert(0 <= size);
  num_workers = size;

  /* handle configuration */
  if (cfg != NULL) {
    /* copy configuration for local use */
    config = *cfg;
  } else {
    config.node = -1;
    config.do_print_workerinfo = 1;
  }

  /* allocate the array of worker contexts */
  workers = (workerctx_t *) malloc( num_workers * sizeof(workerctx_t) );

  /* prepare data structures */
  for( i=0; i<num_workers; i++) {
    workerctx_t *wc = &workers[i];
    char wname[24];
    wc->wid = i;

    wc->wait_cnt = 0;
    TimingZero( &wc->wait_time);

    wc->num_tasks = 0;
    wc->terminate = 0;

    wc->sched = SchedCreate( i);
    wc->wraptask = NULL;

    snprintf( wname, 24, "mon_worker%02d.log", i);
    wc->mon = LpelMonContextCreate( wname);
    
    /* taskqueue of free tasks */
    //TaskqueueInit( &wc->free_tasks);

    /* mailbox */
    MailboxInit( &wc->mailbox);
  }
}



void LpelWorkerSpawn(void)
{
  int i;
  /* create worker threads */
  for( i=0; i<num_workers; i++) {
    workerctx_t *wc = &workers[i];
    /* spawn joinable thread */
    (void) pthread_create( &wc->thread, NULL, WorkerThread, wc);
  }
}


/**
 * Cleanup worker contexts
 *
 */
void LpelWorkerCleanup(void)
{
  int i;
  workerctx_t *wc;

  /* wait on workers */
  for( i=0; i<num_workers; i++) {
    wc = &workers[i];
    /* wait for the worker to finish */
    (void) pthread_join( wc->thread, NULL);
  }
  /* cleanup the data structures */
  for( i=0; i<num_workers; i++) {
    wc = &workers[i];
    MailboxCleanup( &wc->mailbox);
    SchedDestroy( wc->sched);
  }

  /* free memory */
  free( workers);
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
      whom->state = TASK_READY;
      SchedMakeReady( wc->sched, whom);
    }
  }
}


/**
 * Get a worker context from the worker id
 */
workerctx_t *LpelWorkerGetContext(int id) {
  if (id >= 0 && id < num_workers) {
    return &workers[id];
  }

  /* create a new worker context for a wrapper */
  if (id == -1) {
    workerctx_t *wc = (workerctx_t *) malloc( sizeof( workerctx_t));
    wc->wid = -1;
    wc->terminate = 0;
    wc->wait_cnt = 0;
    TimingZero( &wc->wait_time);
    /* Wrapper is excluded from scheduling module */
    wc->sched = NULL;
    wc->wraptask = NULL;
    wc->mon = NULL;
    /* taskqueue of free tasks */
    //TaskqueueInit( &wc->free_tasks);
    /* mailbox */
    MailboxInit( &wc->mailbox);
    (void) pthread_create( &wc->thread, NULL, WorkerThread, wc);
    (void) pthread_detach( wc->thread);

    return wc;
  }
  return NULL;
}






/******************************************************************************/
/*  PRIVATE FUNCTIONS                                                         */
/******************************************************************************/


/**
 * Reschedule workers own task, returning from a call
 */
static void RescheduleTask( workerctx_t *wc, lpel_task_t *t)
{
  /* reschedule task */
  switch(t->state) {

    /* task exited by calling TaskExit() */
    case TASK_ZOMBIE:
      //FIXME LpelMonitoringDebug( wc->mon, "Killed task %d.\n", t->uid);
      LpelTaskDestroy( t);
      wc->num_tasks--;


      /* wrappers can terminate if their task terminates */
      if (wc->wid < 0) {
        wc->terminate = 1;
      }
      break;

    /* task returned from a blocking call*/
    case TASK_BLOCKED:
      /* do nothing */
      break;

    /* task yielded execution  */
    case TASK_READY:
      if (wc->wid < 0) {
        SendWakeup( wc, t);
      } else {
        SchedMakeReady( wc->sched, t);
      }
      break;

    /* should not be reached */
    default: assert(0);
  }
}



static void ProcessMessage( workerctx_t *wc, workermsg_t *msg)
{
  lpel_task_t *t;
  
  //FIXME LpelMonitoringDebug( wc->mon, "worker %d processing msg %d\n", wc->wid, msg->type);

  switch( msg->type) {
    case WORKER_MSG_WAKEUP:
      /* worker has new ready tasks,
       * just wakeup to continue loop
       */
      t = msg->body.task;
      assert(t->state == TASK_BLOCKED);
      t->state = TASK_READY;
      //FIXME LpelMonitoringDebug( wc->mon, "Received wakeup for %d.\n", t->uid);
      if (wc->wid < 0) {
        wc->wraptask = t;
      } else {
        SchedMakeReady( wc->sched, t);
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
      //FIXME LpelMonitoringDebug( wc->mon, "Assigned task %d.\n", t->uid);
      
      if (wc->wid < 0) {
        wc->wraptask = t;
        /* create monitoring context if necessary */
        if (t->mon) {
          char fname[32];
          char *taskname = LpelMonTaskGetName(t->mon);
          (void) snprintf(fname, 32, "mon_%.24s.log", taskname);
          wc->mon = LpelMonContextCreate(fname);
          /* start message */
          LpelMonDebug( wc->mon, "Wrapper %s started.\n", taskname);
        }
      } else {
        SchedMakeReady( wc->sched, t);
      }
      /* assign monitoring context to taskmon */
      if (t->mon) LpelMonTaskAssign(t->mon, wc->mon);
      break;
    default: assert(0);
  }
}


static void WaitForNewMessage( workerctx_t *wc)
{
  workermsg_t msg;
  timing_t wtm;

  wc->wait_cnt += 1;
  
  TimingStart( &wtm);
  MailboxRecv( &wc->mailbox, &msg);
  TimingEnd( &wtm);
  
  //FIXME
  /*
  LpelMonitoringDebug( wc->mon,
      "worker %d waited (%u) for %lu.%09lu\n",
      wc->wid,
      wc->wait_cnt,
      (unsigned long) wtm.tv_sec, wtm.tv_nsec
      );
  */

  TimingAdd( &wc->wait_time, &wtm);
  
  ProcessMessage( wc, &msg);
}


static void FetchAllMessages( workerctx_t *wc)
{
  workermsg_t msg;
  while( MailboxHasIncoming( &wc->mailbox) ) {
    MailboxRecv( &wc->mailbox, &msg);
    ProcessMessage( wc, &msg);
  }
}


/**
 * Worker loop
 */
static void WorkerLoop( workerctx_t *wc)
{
  lpel_task_t *t = NULL;
  
  /* start message */
  LpelMonDebug( wc->mon, "Worker %d started.\n", wc->wid);

  do {
    t = SchedFetchReady( wc->sched);
    if (t != NULL) {

      /* execute task */
      LpelWorkerTaskCall( wc, t);
      
      //FIXME LpelMonitoringDebug( wc->mon, "Back on worker %d context.\n", wc->wid);

      assert( wc->predecessor != NULL);
      
      LpelWorkerFinalizeTask( wc);
    } else {
      /* no ready tasks */
      WaitForNewMessage( wc);
    }
    /* fetch (remaining) messages */
    FetchAllMessages( wc);
  } while ( !( 0==wc->num_tasks && wc->terminate) );
  //} while ( !wc->terminate);

}


static void WrapperLoop( workerctx_t *wc)
{
  lpel_task_t *t = NULL;

  /*
   * Create a monitoring context for that wrapper,
   * by the mon_info of the task
   */
   //TODO

  do {
    t = wc->wraptask;
    if (t != NULL) {

      /* execute task */
      LpelWorkerTaskCall( wc, t);

      wc->wraptask = NULL;
      assert( wc->predecessor != NULL);
      
      LpelWorkerFinalizeTask( wc);
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

  /* Init libPCL */
  co_thread_init();
  wc->mctx = co_current();


  /* no predecessor */
  wc->predecessor = NULL;

  /* assign to cores */
  LpelThreadAssign( wc->wid);

  /*******************************************************/
  if ( wc->wid >= 0) {
    WorkerLoop( wc);
  } else {
    WrapperLoop( wc);
  }
  /*******************************************************/
  
  if (wc->mon) {
    /* exit message */
    LpelMonDebug( wc->mon,
        "Worker %d exited. wait_cnt %u, wait_time %lu.%09lu\n",
        wc->wid,
        wc->wait_cnt, 
        (unsigned long) wc->wait_time.tv_sec, wc->wait_time.tv_nsec
        );

    /* cleanup monitoring */
    LpelMonContextDestroy( wc->mon);
  }
  

  /* destroy all the free tasks */
  /*
  while( wc->free_tasks.count > 0) {
    lpel_task_t *t = TaskqueuePopFront( &wc->free_tasks);
    free( t);
  }
  */

  /* on a wrapper, we also can cleanup more*/
  if (wc->wid < 0) {
    /* clean up the mailbox for the worker */
    MailboxCleanup( &wc->mailbox);

    /* free the worker context */
    free( wc);
  }
  /* Cleanup libPCL */
  co_thread_cleanup();

  return NULL;
}

