/**
 * The LPEL worker containing code for workers and wrappers
 */

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <pthread.h>
#include <assert.h>
#include <errno.h>

#include "arch/timing.h"
#include "arch/atomic.h"

#include "worker.h"

#include "bool.h"
#include "lpel.h"

#include "task.h"


#include "monitoring.h"


static int num_workers = -1;
static workerctx_t *workers;
static workercfg_t config;



static void *StartupThread( void *arg);

static void MsgSend( workerctx_t *wc, workermsg_t *msg)
{
  pthread_mutex_lock( &wc->lock_inbox);
  if ( wc->list_inbox == NULL) {
    /* list is empty */
    wc->list_inbox = msg;
    msg->next = msg; /* self-loop */

    pthread_cond_signal( &wc->notempty);

  } else { 
    /* insert stream between last msg=list_inbox
       and first msg=list_inbox->next */
    msg->next = wc->list_inbox->next;
    wc->list_inbox->next = msg;
    wc->list_inbox = msg;
  }
  pthread_mutex_unlock( &wc->lock_inbox);
}


static workermsg_t *MsgRecv( workerctx_t *wc)
{
  workermsg_t *msg;

  pthread_mutex_lock( &wc->lock_inbox);
  while( wc->list_inbox == NULL) {
      pthread_cond_wait( &wc->notempty, &wc->lock_inbox);
  }

  assert( wc->list_inbox != NULL);

  msg = wc->list_inbox->next;
  if ( msg == wc->list_inbox) {
    /* self-loop, just single msg*/
    wc->list_inbox = NULL;
  } else {
    wc->list_inbox->next = msg->next;
  }
  pthread_mutex_unlock( &wc->lock_inbox);

  return msg;
}

/**
 * @note: does not need to be locked as a 'missed' msg 
 *        will be eventually fetched in the next worker loop
 */
static inline bool MsgHasIncoming( workerctx_t *wc)
{
  return ( wc->list_inbox != NULL);
}


static workermsg_t *MsgGetFree( workerctx_t *wc)
{
  workermsg_t *msg;

  pthread_mutex_lock( &wc->lock_free);
  if (wc->list_free != NULL) {
    /* pop free message off */
    msg = wc->list_free;
    wc->list_free = msg->next; /* can be NULL */
  } else {
    msg = (workermsg_t *)malloc( sizeof( workermsg_t));
  }
  pthread_mutex_unlock( &wc->lock_free);

  return msg;
}

static void MsgPutFree( workerctx_t *wc, workermsg_t *msg)
{
  pthread_mutex_lock( &wc->lock_free);
  if ( wc->list_free == NULL) {
    msg->next = NULL;
  } else {
    msg->next = wc->list_free;
  }
  wc->list_free = msg;
  pthread_mutex_unlock( &wc->lock_free);
}



static inline void SendTerminate( workerctx_t *target)
{
  /* get message from free list */
  workermsg_t *msg = MsgGetFree( target);
  /* compose a task term message */
  msg->type = WORKER_MSG_TERMINATE;
  /* send */
  MsgSend( target, msg);
}

static inline void SendWakeup( workerctx_t *target, task_t *t)
{
  /* get message from free list */
  workermsg_t *msg = MsgGetFree( target);
  /* compose a task wakeup message */
  msg->type = WORKER_MSG_WAKEUP;
  msg->task = t;
  /* send */
  MsgSend( target, msg);
}

static inline void SendAssign( workerctx_t *target, task_t *t)
{
  /* get message from free list */
  workermsg_t *msg = MsgGetFree( target);
  /* compose a task assign message */
  msg->type = WORKER_MSG_ASSIGN;
  msg->task = t;
  /* send */
  MsgSend( target, msg);
}







/**
 * Initialise worker globally
 *
 *
 * @param size  size of the worker set, i.e., the total number of workers
 * @param cfg   additional configuration
 */
void WorkerInit(int size, workercfg_t *cfg)
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
    config.do_print_workerinfo = false;
    config.task_info_print = NULL;
  }

  SchedInit( num_workers);

  /* allocate the array of worker contexts */
  workers = (workerctx_t *) malloc( num_workers * sizeof(workerctx_t) );

  /* create worker threads */
  for( i=0; i<num_workers; i++) {
    workerctx_t *wc = &workers[i];
    char wname[11];
    wc->wid = i;
    wc->num_tasks = 0;
    wc->terminate = false;

    wc->sched = SchedCreate( i);

    snprintf( wname, 11, "worker%02d", i);
    wc->mon = MonitoringCreate( config.node, wname);

    /* mailbox */
    pthread_mutex_init( &wc->lock_free,  NULL);
    pthread_mutex_init( &wc->lock_inbox, NULL);
    pthread_cond_init(  &wc->notempty,   NULL);
    wc->list_free  = NULL;
    wc->list_inbox = NULL;

    /* spawn joinable thread */
    (void) pthread_create( &wc->thread, NULL, StartupThread, wc);
  }
}


/**
 * Create a wrapper thread for a single task
 */
void WorkerWrapperCreate(task_t *t, char *name)
{
  assert(name != NULL);

  /* create worker context */
  workerctx_t *wc = (workerctx_t *) malloc( sizeof( workerctx_t));

  wc->wid = -1;
  wc->num_tasks = 0;
  wc->terminate = false;

  wc->sched = SchedCreate( -1);

  wc->mon = MonitoringCreate( config.node, name);

  /* mailbox */
  pthread_mutex_init( &wc->lock_free,  NULL);
  pthread_mutex_init( &wc->lock_inbox, NULL);
  pthread_cond_init(  &wc->notempty,   NULL);
  wc->list_free  = NULL;
  wc->list_inbox = NULL;

  /* send assign message for the task */
  SendAssign( wc, t);

  /* send terminate message afterwards, as this is the only task */
  SendTerminate( wc);


 (void) pthread_create( &wc->thread, NULL, StartupThread, wc);
 (void) pthread_detach( wc->thread);
}



void WorkerTerminate(void)
{
  int i;
  workerctx_t *wc;

  /* signal the workers to terminate */
  for( i=0; i<num_workers; i++) {
    wc = &workers[i];
    
    /* send a termination message */
    SendTerminate( wc);
  }

}

/**
 * Cleanup worker contexts
 *
 */
void WorkerCleanup(void)
{
  int i;

  /* wait on workers */
  for( i=0; i<num_workers; i++) {
    workerctx_t *wc = &workers[i];
    /* wait for the worker to finish */
    (void) pthread_join( wc->thread, NULL);
  }

  /* cleanup scheduler */
  SchedCleanup();

  /* free memory */
  free( workers);
}



void WorkerTaskWakeup( task_t *by, task_t *whom)
{
  workerctx_t *wc = whom->worker_context;
  if ( wc == by->worker_context) {
    SchedMakeReady( wc->sched, whom);
  } else {
    SendWakeup( wc, whom);
  }
}




/**
 * Assign a task to the worker
 */
void WorkerTaskAssign( task_t *t, int wid)
{
  workerctx_t *wc = &workers[wid];

  SendAssign( wc, t);
}




static void RescheduleTask( workerctx_t *wc, task_t *t)
{
  /* reworkerule task */
  switch(t->state) {
    case TASK_ZOMBIE:  /* task exited by calling TaskExit() */
      TaskDestroy( t);
      wc->num_tasks -= 1;
      break;
    case TASK_BLOCKED: /* task returned from a blocking call*/
      /* do nothing */
      break;
    case TASK_READY: /* task yielded execution  */
      SchedMakeReady( wc->sched, t);
      break;
    default: assert(0); /* should not be reached */
  }
}


static void ProcessMessage( workerctx_t *wc, workermsg_t *msg)
{
  switch( msg->type) {
    case WORKER_MSG_TERMINATE:
      wc->terminate = true;
      break;
    case WORKER_MSG_ASSIGN:
      wc->num_tasks += 1;
      /* set worker context of task */
      msg->task->worker_context = wc;
    case WORKER_MSG_WAKEUP:
      SchedMakeReady( wc->sched, msg->task);
      break;
    default: assert(0);
  }
  MsgPutFree( wc, msg);
}

static void FetchAllMessages( workerctx_t *wc)
{
  workermsg_t *msg;
  while( MsgHasIncoming( wc) ) {
    msg = MsgRecv( wc);
    ProcessMessage( wc, msg);
  }
}

/******************************************************************************/
/*  WORKER                                                                    */
/******************************************************************************/


static void WorkerLoop( workerctx_t *wc)
{
  task_t *t;

  wc->loop = 0;
  do {
    t = SchedFetchReady( wc->sched);
    if (t != NULL) {
      /* increment counter for next loop */
      wc->loop++;

      /* execute task */
      TaskCall(t);
      
      RescheduleTask( wc, t);
    } else {
      /* wait for a new message and process */
      workermsg_t *msg = MsgRecv( wc);
      ProcessMessage( wc, msg);
    }
    /* fetch (remaining) messages */
    FetchAllMessages( wc);
  } while ( !(wc->num_tasks==0 && wc->terminate) );

  MonitoringDebug( wc->mon, "Worker exited.");
}




/**
 * Startup thread for workers and wrappers
 */
static void *StartupThread( void *arg)
{
  workerctx_t *wc = (workerctx_t *)arg;
  
  /* Init libPCL */
  co_thread_init();

  /* assign to cores */
  LpelThreadAssign( wc->wid);

  /* call worker function */
  WorkerLoop( wc);
  
  /* cleanup monitoring */
  MonitoringDestroy( wc->mon);
  
  SchedDestroy( wc->sched);

  /* clean up the mailbox for the worker */
  pthread_mutex_destroy( &wc->lock_free);
  pthread_mutex_destroy( &wc->lock_inbox);
  pthread_cond_destroy(  &wc->notempty);
  
  /* free the worker context */
  if (wc->wid < 0) free( wc);

  /* Cleanup libPCL */
  co_thread_cleanup();

  return NULL;
}


