/**
 * The LPEL worker containing code for workers and wrappers
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <errno.h>

#ifdef MEASUREMENTS
#include <time.h>
#endif

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

#define WORKER_PTR(i) (workers[(i)])




/******************************************************************************/

#ifdef MEASUREMENTS
static pthread_mutex_t measure_mutex;
static long max_time;
static long min_time;
static long total_tasks;
#endif

static int num_workers = -1;
static workerctx_t **workers;

static pthread_mutex_t *mutex_workers;


#ifdef HAVE___THREAD
static __thread workerctx_t *workerctx_cur;
#else /* HAVE___THREAD */
static pthread_key_t workerctx_key;
#endif /* HAVE___THREAD */


/* worker thread function declaration */
static void *WorkerThread( void *arg);


static void FetchAllMessages( workerctx_t *wc);
static void CleanupTaskContext(workerctx_t *wc, lpel_task_t *t);




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
void LpelWorkerInit(lpel_config_t *config)
{
  int i, res, size;

  size = config->num_workers;

  assert(0 <= size);
  num_workers = size;

#ifndef HAVE___THREAD
  /* init key for thread specific data */
  pthread_key_create(&workerctx_key, NULL);
#endif /* HAVE___THREAD */

#ifdef MEASUREMENTS
  pthread_mutex_init(&measure_mutex, NULL);
  max_time = 0;
  min_time = 0;
#endif

  /* initialize spmdext module */
  res = LpelSpmdInit(num_workers);

  /* allocate worker context table */
  workers = (workerctx_t **) malloc( num_workers * sizeof(workerctx_t*) );

  /* allocate memory for mutexes for each worker*/
  mutex_workers = malloc(num_workers * sizeof(pthread_mutex_t));
  /* allocate worker contexts  and mutexes*/
  for (i=0; i<num_workers; i++) {
    workers[i] = (workerctx_t *) malloc( sizeof(workerctx_t) );

    pthread_mutex_init(&mutex_workers[i], NULL);
  }

  /* prepare data structures */
  for( i=0; i<num_workers; i++) {
    workerctx_t *wc = WORKER_PTR(i);
    wc->wid = i;
    wc->num_tasks = 0;
    wc->terminate = 0;

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

    /* taskqueue of free tasks */
    //LpelTaskqueueInit( &wc->free_tasks);

  }

  /* Initialize placement scheduler */
  LpelPlacementSchedulerInit(config);

  assert(res==0);
}


/**
 * Cleanup worker contexts
 *
 */
void LpelWorkerCleanup(void)
{
  int i;
  workerctx_t *wc;
#ifdef MEASUREMENTS
  struct timespec res_spec;
#endif

  /* wait on workers */
  for( i=0; i<num_workers; i++) {
    wc = WORKER_PTR(i);
    /* wait for the worker to finish */
    (void) pthread_join( wc->thread, NULL);
  }
  /* cleanup the data structures */
  for( i=0; i<num_workers; i++) {
    pthread_mutex_lock(&mutex_workers[i]);
    wc = WORKER_PTR(i);
    LpelMailboxDestroy(wc->mailbox);
    LpelSchedDestroy( wc->sched);
    free(wc);
    pthread_mutex_unlock(&mutex_workers[i]);

    pthread_mutex_destroy(&mutex_workers[i]);
  }

  /* free workers table */
  free( workers);

  /* free mutex table */
  free(mutex_workers);

#ifdef MEASUREMENTS
  clock_getres(CLOCK_ID, &res_spec);
  pthread_mutex_destroy(&measure_mutex);

  printf("WORKER STATISTICS:\n");
  printf("RESOLUTION:\t%ld.%ld\n", res_spec.tv_sec, res_spec.tv_nsec);
  printf("MINIMUM TIME:\t%lf\n", (double)min_time / (double)1000000000);
  printf("MAXIMUM TIME:\t%lf\n", (double)max_time / (double)1000000000);
  printf("TOTAL NUMBER OF TASKS:\t%ld\n", total_tasks);
#endif

  /* destroy placement scheduler */
  LpelPlacementSchedulerDestroy();

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
    /* taskqueue of free tasks */
    //LpelTaskqueueInit( &wc->free_tasks);
    (void) pthread_create( &wc->thread, NULL, WorkerThread, wc);
    (void) pthread_detach( wc->thread);
  }

  assert(wc != NULL);
  return wc;
}



void LpelWorkerSelfTaskExit(lpel_task_t *t)
{
  workerctx_t *wc = t->worker_context;

  CleanupTaskContext(wc,t);
  wc->num_tasks--;
  /* wrappers can terminate if their task terminates */
  if (wc->wid < 0) {
    wc->terminate = 1;
  }
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

/**
 * Returns the number of workers excluding the worker which runs
 * the placement scheduler.
 */
int LpelWorkerNumber()
{
#ifdef SCHEDULER_CONCURRENT_PLACEMENT
  return num_workers-1;
#else
  return num_workers;
#endif
}

workerctx_t **LpelWorkerGetWorkers()
{
  return workers;
}

pthread_mutex_t *LpelWorkerGetMutexes()
{
  return mutex_workers;
}

#ifdef TASK_SEGMENTATION
void LpelWorkerSetTaskType(int wid, int type)
{
  workers[wid]->task_type = type;
}
#endif

#ifdef MEASUREMENTS
void LpelWorkerAddTask()
{
  pthread_mutex_lock(&measure_mutex);
  total_tasks++;
  pthread_mutex_unlock(&measure_mutex);
}
#endif

/******************************************************************************/
/*  PRIVATE FUNCTIONS                                                         */
/******************************************************************************/

/**
 * Deferred deletion of a task
 */
static void CleanupTaskContext(workerctx_t *wc, lpel_task_t *t)
{
  /* delete task marked before */
  if (wc->marked_del != NULL) {
    //LpelMonDebug( wc->mon, "Destroy task %d\n", wc->marked_del->uid);
    LpelTaskDestroy( wc->marked_del);
    wc->marked_del = NULL;
  }
  /* place a new task (if any) */
  if (t != NULL) {
    wc->marked_del = t;
  }
}




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
#ifdef WAITING
      LpelTimingStart(&t->last_measurement_start);
#endif
      t->state = TASK_READY;

      WORKER_DBGMSG(wc, "Received wakeup for %d.\n", t->uid);

      if (wc->wid < 0) {
        wc->wraptask = t;
      } else {
        LpelSchedMakeReady( wc->sched, t);
      }
      break;

    case WORKER_MSG_TERMINATE:
      pthread_mutex_lock(&mutex_workers[wc->wid]);
      wc->terminate = 1;
      pthread_mutex_unlock(&mutex_workers[wc->wid]);
      break;

    case WORKER_MSG_ASSIGN:
      t = msg->body.task;

      assert(t->state == TASK_CREATED);
#ifdef WAITING
      LpelTimingStart(&t->last_measurement_start);
#endif
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
#ifdef MEASUREMENTS
        struct timespec stop_time;
        long time;
#endif
        LpelSchedMakeReady( wc->sched, t);
#ifdef MEASUREMENTS
        clock_gettime(CLOCK_ID, &stop_time);
        time = (stop_time.tv_sec - t->start_time.tv_sec) * 1000000000 +
               (stop_time.tv_nsec - t->start_time.tv_nsec);
        pthread_mutex_lock(&measure_mutex);
        max_time = (time > max_time) ? time : max_time;
        min_time = (time < min_time || min_time == 0) ? time : max_time;
        total_tasks++;
        pthread_mutex_unlock(&measure_mutex);
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
  //lpel_task_iterator_t *iter = NULL;

  do {
    /* before executing a task, handle all pending requests! */
    LpelSpmdHandleRequests(wc->wid);

    t = LpelSchedFetchReady( wc->sched);
    if (t != NULL) {
      /* execute task */
      wc->current_task = t;
      mctx_switch(&wc->mctx, &t->mctx);


      WORKER_DBGMSG(wc, "Back on worker %d context.\n", wc->wid);

      /* cleanup task context marked for deletion */
      CleanupTaskContext(wc, NULL);
    } else {
#ifdef WAITING
      pthread_mutex_lock(&mutex_workers[wc->wid]);
      wc->waiting = 1;
      pthread_mutex_unlock(&mutex_workers[wc->wid]);
#endif
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

  /* cleanup task context marked for deletion */
  CleanupTaskContext(wc, NULL);
}


/**
 * Thread function for workers (and wrappers)
 */
static void *WorkerThread( void *arg)
{
  workerctx_t *wc = (workerctx_t *)arg;

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
  /*
  while( wc->free_tasks.count > 0) {
    lpel_task_t *t = LpelTaskqueuePopFront( &wc->free_tasks);
    free( t);
  }
  */

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
