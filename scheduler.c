/**
 * A simple scheduler
 */

#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <sched.h>

#include "lpel.h"
#include "scheduler.h"
#include "timing.h"
#include "monitoring.h"
#include "atomic.h"
#include "taskqueue.h"

#include "stream.h"
#include "buffer.h"


struct schedcfg {
  int dummy;
};




struct schedctx {
  int wid; 
};


static struct {
  taskqueue_t     queue;
  pthread_mutex_t lock;
  pthread_cond_t  activate;
  bool terminate;  
} sched_global;





static int num_workers = -1;
static schedctx_t *schedarray;



static void PutQueue( task_t *t);

static void Reschedule(schedctx_t *sc, task_t *t);




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

  assert(0 <= size);

  num_workers = size;
  
  /* allocate the array of scheduler data */
  schedarray = (schedctx_t *) calloc( size, sizeof(schedctx_t) );

  
  /* init global data */
  TaskqueueInit( &sched_global.queue);
  pthread_mutex_init( &sched_global.lock);
  pthread_cond_init( &sched_global.activate);
  sched_global.terminate = false;
}


schedctx_t *SchedContextCreate( int wid)
{
  schedctx_t *sc = &schedarray[wid];

  sc->wid = wid;

  return sc;
}


void SchedContextDestroy( schedctx_t *sc)
{
  /* do nothing */
}



/**
 * Cleanup scheduler data
 *
 */
void SchedCleanup(void)
{
  assert( sched_global.terminate == true );
  
  /* destroy globals */
  pthread_mutex_destroy( &sched_global.lock);
  pthread_cond_destroy( &sched_global.activate);

  /* free memory for scheduler data */
  free( schedarray);
}



void SchedWakeup( task_t *by, task_t *whom)
{
  PutQueue( whom);
}



/**
 * Assign a task to the scheduler
 */
void SchedAssignTask( schedctx_t *sc, task_t *t)
{
  PutQueue( t);
}





/**
 * Main worker thread
 *
 */
void SchedThread( lpelthread_t *env, void *arg)
{
  schedctx_t *sc = (schedctx_t *)arg;
  unsigned int loop;
  bool terminate;
  task_t *t;  

  /* MAIN SCHEDULER LOOP */
  loop=0;
  do {

    /* monitor enter */
    pthread_mutex_lock( &sched_global.lock);
    terminate = sched_global.terminate;
    while( 0 == sched_global.queue.count && !terminate) {
      pthread_cond_wait( &sched_global.activate, &sched_global.lock);
      terminate = sched_global.terminate;
    }
    t = TaskqueuePopFront( &sched_global.queue);
    pthread_mutex_unlock( &sched_global.lock);
    /* monitor exit */
    
    if (t != NULL) {
      /* execute task */
      pthread_mutex_lock( t->lock);
      t->cnt_dispatch++;

      TIMESTAMP(&t->times.start);
      TaskCall(t);
      TIMESTAMP(&t->times.stop);
      /* task returns in every case in a different state */
      assert( t->state != TASK_RUNNING);

      /* output accounting info */
      MonPrintTask(env, t);
      MonWorkerDbg(env, "(worker %d, loop %u)\n", sc->wid, loop);

      Reschedule(sc, t);
      pthread_mutex_unlock( t->lock);
    } /* end if executed ready task */


    loop++;
  } while ( !terminate );
  /* stop only if there are no more tasks in the system */
  /* MAIN SCHEDULER LOOP END */
  
}






/** PRIVATE FUNCTIONS *******************************************************/


static void PutQueue( task_t *t)
{
  /* put into ready queue */
  pthread_mutex_lock( &sched_global.lock );
  TaskqueueEnqueue( &sched_global.queue, t );
  if ( 1 == &sched_global.queue.count ) {
    pthread_cond_signal( &sched_global.activate );
  }
  pthread_mutex_unlock( &sched_global.lock );
}

/**
 * Reschedule a task after execution
 *
 * @param sc   scheduler context
 * @param t    task returned from execution in a different state than READY
 */
static void Reschedule(schedctx_t *sc, task_t *t)
{
  /* check state of task, place into appropriate queue */
  switch(t->state) {
    case TASK_ZOMBIE:  /* task exited by calling TaskExit() */
      /*TODO handle joinable tasks */
      TaskDestroy( t);
      break;

    case TASK_WAITING: /* task returned from a blocking call*/
      /* do nothing */
      break;

    case TASK_READY: /* task yielded execution  */
      /* put into ready queue */
      PutQueue( t);
      break;

    default:
      assert(0); /* should not be reached */
  }
}


