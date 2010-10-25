/**
 * A simple scheduler
 */

#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <errno.h>

#include "lpel.h"
#include "scheduler.h"
#include "timing.h"
#include "monitoring.h"
#include "atomic.h"
#include "bqueue.h"

#include "task.h"
#include "stream.h"


struct schedcfg {
  int dummy;
};



#define SCTYPE_WORKER  0
#define SCTYPE_WRAPPER 1


struct schedctx {
  int type;
  lpelthread_t *env;
  union {
    struct {
      bqueue_t rq;
    } wrapper;
    struct {
      int wid; 
    } worker;
  } ctx;
};



static bqueue_t sched_global;

static int num_workers = -1;

static schedctx_t *workers;



/* static function prototypes */
static void SchedWorker( lpelthread_t *env, void *arg);


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
  
  
  /* init global ready queue */
  BQueueInit( &sched_global);

    
  /* allocate the array of scheduler data */
  workers = (schedctx_t *) malloc( num_workers * sizeof(schedctx_t) );

  /* create worker threads */
  for( i=0; i<num_workers; i++) {
    schedctx_t *sc = &workers[i];
    char wname[11];
    sc->type = SCTYPE_WORKER;
    sc->ctx.worker.wid = i;
    snprintf( wname, 11, "worker%02d", i);
    /* aquire thread */ 
    sc->env = LpelThreadCreate( SchedWorker, (void*)sc, false, wname);
  }
}


/**
 * Cleanup scheduler data
 *
 */
void SchedCleanup(void)
{
  int i;

  /* wait for the workers to finish */
  for( i=0; i<num_workers; i++) {
    LpelThreadJoin( workers[i].env);
  }

  /* free memory for scheduler data */
  free( workers);
  
  /* destroy global ready queue */
  BQueueCleanup( &sched_global);
}






void SchedWakeup( task_t *by, task_t *whom)
{
  /* dependent on whom, put in global or wrapper queue */
  schedctx_t *sc = whom->sched_context;
  
  if ( sc->type == SCTYPE_WORKER) {
    BQueuePut( &sched_global, whom);
  } else if ( sc->type == SCTYPE_WRAPPER) {
    BQueuePut( &sc->ctx.wrapper.rq, whom);
  } else {
    assert(0);
  }
}



/**
 * Assign a task to the scheduler
 */
void SchedAssignTask( task_t *t)
{
  BQueuePut( &sched_global, t);
}





/**
 * Main worker thread
 *
 */
static void SchedWorker( lpelthread_t *env, void *arg)
{
  schedctx_t *sc = (schedctx_t *)arg;
  unsigned int loop, delayed;
  int wid;
  bool terminate;
  task_t *t;  

  wid = sc->ctx.worker.wid;

  /* assign to core */
  LpelThreadAssign( env, wid % num_workers );

  /* MAIN SCHEDULER LOOP */
  loop=0;
  do {
    terminate = BQueueFetch( &sched_global, &t);
    if (t != NULL) {
      
      /* aqiure task lock, count congestion */
      if ( EBUSY == pthread_mutex_trylock( &t->lock)) {
        delayed++;
        pthread_mutex_lock( &t->lock);
      }

      t->sched_context = sc;
      t->cnt_dispatch++;

      /* execute task */
      TIMESTAMP(&t->times.start);
      TaskCall(t);
      TIMESTAMP(&t->times.stop);
      
      /* task returns in every case in a different state */
      assert( t->state != TASK_RUNNING);

      /* output accounting info */
      //MonTaskPrint(env, t);
      MonDebug(env, "(worker %d, loop %u)\n", wid, loop);

      /* check state of task, place into appropriate queue */
      switch(t->state) {
        case TASK_ZOMBIE:  /* task exited by calling TaskExit() */
          TaskDestroy( t);
          break;

        case TASK_WAITING: /* task returned from a blocking call*/
          /* do nothing */
          break;

        case TASK_READY: /* task yielded execution  */
          assert( sc->type == SCTYPE_WORKER);
          BQueuePut( &sched_global, t);
          break;

        default: assert(0); /* should not be reached */
      }
      pthread_mutex_unlock( &t->lock);
    } /* end if executed ready task */
    loop++;
  } while ( !terminate );
  /* stop only if there are no more tasks in the system */
  /* MAIN SCHEDULER LOOP END */
}



void SchedWrapper( lpelthread_t *env, void *arg)
{
  task_t *t = (task_t *)arg;
  unsigned int loop;
  bool terminate;
  bqueue_t *rq;
  schedctx_t *sc;

  /* create scheduler context */
  sc = (schedctx_t *) malloc( sizeof( schedctx_t));
  sc->type = SCTYPE_WRAPPER;
  sc->env = env;
  rq = &sc->ctx.wrapper.rq;
  BQueueInit( rq);

  /* assign scheduler context */
  t->sched_context = sc;


  /* assign to others */
  LpelThreadAssign( env, -1 );

  /* MAIN SCHEDULER LOOP */
  loop=0;
  do {
    (void) BQueueFetch( rq, &t);
    assert(t != NULL);
    
    t->cnt_dispatch++;

    /* execute task */
    TIMESTAMP(&t->times.start);
    TaskCall(t);
    TIMESTAMP(&t->times.stop);
      
    /* task returns in every case in a different state */
    assert( t->state != TASK_RUNNING);

    /* output accounting info */
    //MonTaskPrint(env, t);
    MonDebug(env, "(loop %u)\n", loop);

    /* check state of task, place into appropriate queue */
    switch(t->state) {
      case TASK_ZOMBIE:  /* task exited by calling TaskExit() */
        terminate = true;
        TaskDestroy( t);
        break;

      case TASK_WAITING: /* task returned from a blocking call*/
        /* do nothing */
        break;

      case TASK_READY: /* task yielded execution  */
        assert( sc->type == SCTYPE_WRAPPER);
        BQueuePut( &sc->ctx.wrapper.rq, t);
        break;

      default: assert(0); /* should not be reached */
    }
    loop++;
  } while ( !terminate );

  /* free the scheduler context */
  free( sc);
}


/** PRIVATE FUNCTIONS *******************************************************/

