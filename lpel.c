/**
 * Main LPEL module
 * contains:
 * - startup and shutdown routines
 * - worker thread code
 *
 * TODO: adopt Michael & Scott's two lock queue, s.t.
 * worker reading init queue does not need a lock,
 * only concurrent enqueuers need to lock
 *
 */

#include <malloc.h>
#include <assert.h>

#include <pthread.h> /* worker threads are pthreads (linux has 1-1 mapping) */
#include <pcl.h>     /* tasks are executed in user-space with help of
                        GNU Portable Coroutine Library  */

#include "lpel.h"

#include "task.h"
#include "taskqueue.h"
#include "set.h"
#include "stream.h"
#include "debug.h"

/* used (imported) modules of LPEL */
#include "cpuassign.h"
#include "timing.h"
#include "monitoring.h"
#include "scheduler.h"
#include "atomic.h"


typedef struct {
  taskqueue_t queue_init;         /* init queue */
  pthread_mutex_t mtx_queue_init; /* init lock */
  readyset_t *queue_ready;        /* ready queue */
  taskqueue_t queue_waiting;      /* waiting queue */
  monitoring_t *mon_info;
} workerdata_t;

/* array of workerdata_t, one for each worker */
static workerdata_t *workerdata = NULL;

/*
 * Global task count, i.e. number of tasks in the LPEL.
 * Implemented as atomic unsigned long type.
 */
static atomic_t task_count_global = ATOMIC_INIT(0);

static int num_workers = -1;
static bool b_assigncore = false;

static pthread_key_t worker_id_key;


#define EXPAVG_ALPHA  0.1f

#define TSD_WORKER_ID (*((int *)pthread_getspecific(worker_id_key)))

#define BIT_IS_SET(vec,b)   (( (vec) & (b) ) == (b) )

#if 0
/*
 * Get current executed task
 */
static task_t *LpelGetCurrentTask(void)
{
  return workerdata[TSD_WORKER_ID].current_task;
}
#endif


static bool WaitingTest(task_t *wt)
{
  //DBG("waiting test: task %ld", wt->uid);
  //DBG("event_ptr: %p = %d", wt->event_ptr, *wt->event_ptr);
  return *wt->event_ptr == true;
}

static void WaitingRemove(task_t *wt)
{
  DBG("task %ld waiting->ready", wt->uid);
  wt->state = TASK_READY;
  *wt->event_ptr = false;
  wt->event_ptr = NULL;
  SchedPutReady( workerdata[TSD_WORKER_ID].queue_ready, wt );
}




/**
 * Worker thread code
 */
static void *LpelWorker(void *idptr)
{
  unsigned int loop;
  int id = *((int *)idptr);
  workerdata_t *wd = &workerdata[id];

  DBG("worker %d started", id);

  /* set idptr as thread specific */
  (void) pthread_setspecific(worker_id_key, idptr);

  /* set affinity to id=CPU */
  if (b_assigncore) {
    if (CpuAssignToCore(id)) {
      DBG("worker %d assigned to core", id);
    }
  }
  
  /* Init libPCL */
  co_thread_init();

  /* set scheduling policy */
  wd->queue_ready = SchedInit();

  /* initialise monitoring */
  MonitoringInit(&wd->mon_info, id);

  /* MAIN LOOP */
  loop=0;
  do {
    task_t *t;
    timing_t ts;

    //DBG("worker %d, loop %u", id, loop);

    /* fetch new tasks from init queue, insert into ready queue (sched) */
    pthread_mutex_lock( &wd->mtx_queue_init );
    t = TaskqueueRemove( &wd->queue_init );
    while (t != NULL) {
      assert( t->state == TASK_INIT );
      t->state = TASK_READY;
      SchedPutReady( wd->queue_ready, t );
      /* for next iteration: */
      t = TaskqueueRemove( &wd->queue_init );
    }
    pthread_mutex_unlock( &wd->mtx_queue_init );
    

    /* select a task from the ready queue (sched) */
    t = SchedFetchNextReady( wd->queue_ready );
    if (t != NULL) {
      assert( t->state == TASK_READY );

      /* start timing (mon) */
      TimingStart(&ts);

      /* EXECUTE TASK (context switch) */
      t->cnt_dispatch++;
      t->state = TASK_RUNNING;
      DBG("executing task %lu (worker %d)", t->uid, id);
      co_call(t->ctx);
      DBG("task %lu returned (worker %d)", t->uid, id);
      /* task returns in every case with a different state than RUNNING */
      

      /* end timing (mon) */
      TimingEnd(&ts);
      TimingSet(&t->time_lastrun, &ts);
      TimingAdd(&t->time_totalrun, &ts);
      TimingExpAvg(&t->time_expavg, &ts, EXPAVG_ALPHA);

      /* output accounting info (mon) */
      MonitoringPrint(wd->mon_info, t);

      /* check state of task, place into appropriate queue */
      switch(t->state) {
      case TASK_ZOMBIE:  /* task exited by calling TaskExit() */
        TimingEnd(&t->time_alive);
        /*TODO if joinable, place into join queue, else destroy */
        /*idea: joinable tasks get their refcnt initialized to 2 */
        DBG("calling task %lu destroy (worker %d)", t->uid, id);
        TaskDestroy(t);
        break;

      case TASK_WAITING: /* task returned from a blocking call*/
        /* put into waiting queue */
        DBG("task %lu into waiting (worker %d)", t->uid, id);
        TaskqueueAppend( &wd->queue_waiting, t );
        break;

      case TASK_READY: /* task yielded execution  */
        /* put into ready queue */
        DBG("task %lu into ready (worker %d)", t->uid, id);
        SchedPutReady( wd->queue_ready, t );
        break;

      default:
        assert(0); /* should not be reached */
      }

    } /* end if executed ready task */


    /* iterate through waiting queue, check r/w events */
    TaskqueueIterateRemove( &wd->queue_waiting,
                            WaitingTest, WaitingRemove
                            );

    /*XXX (iterate through nap queue, check alert-time) */

    loop++;
  } while ( atomic_read(&task_count_global) > 0 );
  /* stop only if there are no more tasks in the system */
  /* MAIN LOOP END */
  

  /* cleanup monitoring info */
  MonitoringCleanup(wd->mon_info);
  
  /* cleanup scheduling module */
  SchedCleanup(wd->queue_ready);

  /* Cleanup libPCL */
  co_thread_cleanup();
  
  /* exit thread */
  pthread_exit(NULL);
  //return NULL;
}






/**
 * Initialise the LPEL
 * - if num_workers==-1, determine the number of worker threads automatically
 * - create the data structures for each worker thread
 */
void LpelInit(lpelconfig_t *cfg)
{
  int i, cpus;

  cpus = CpuAssignQueryNumCpus();
  if (cfg->num_workers == -1) {
    /* one available core has to be reserved for IO tasks
       and other system threads */
    num_workers = cpus - 1;
  } else {
    num_workers = cfg->num_workers;
  }
  if (num_workers < 1) num_workers = 1;
  
  /* Exclusive assignment possible? */
  if ( BIT_IS_SET(cfg->flags, LPEL_ATTR_ASSIGNCORE) ) {
    if ( !CpuAssignCanExclusively() ) {
      ;/*TODO emit warning*/
      b_assigncore = false;
    } else {
      if (num_workers > (cpus-1)) {
        ;/*TODO emit warning*/
        b_assigncore = false;
      } else {
        b_assigncore = true;
      }
    }
  }


  /* Create the data structures */
  workerdata = (workerdata_t *) malloc( num_workers*sizeof(workerdata_t) );
  for (i=0; i<num_workers; i++) {
    TaskqueueInit(&workerdata[i].queue_init);
    pthread_mutex_init( &workerdata[i].mtx_queue_init, NULL );

    TaskqueueInit(&workerdata[i].queue_waiting);

  }

  /* Init libPCL */
  co_thread_init();
}

/**
 * Create and execute the worker threads
 * - joins on the worker threads
 */
void LpelRun(void)
{
  pthread_t *thids;
  int i, res;
  int wids[num_workers];

  /* Create thread-specific data key for worker_id */
  pthread_key_create(&worker_id_key, NULL);


  // launch worker threads
  thids = (pthread_t *) malloc(num_workers * sizeof(pthread_t));
  for (i = 0; i < num_workers; i++) {
    //workerdata[i].id = i;
    //res = pthread_create(&thids[i], NULL, LpelWorker, &(workerdata[i].id));
    wids[i] = i;
    res = pthread_create(&thids[i], NULL, LpelWorker, &wids[i]);
    if (res != 0) {
      /*TODO error
      perror("creating worker threads");
      exit(-1);
      */
    }
  }

  // join on finish
  for (i = 0; i < num_workers; i++) {
    pthread_join(thids[i], NULL);
  }
  DBG("workers have finished.");
  pthread_key_delete(worker_id_key);

}


/**
 * Cleans the LPEL up
 * - free the data structures of worker threads
 */
void LpelCleanup(void)
{
  int i;
  for (i=0; i<num_workers; i++) {
    pthread_mutex_destroy( &workerdata[i].mtx_queue_init );
  }
  free(workerdata);

  /* Cleanup libPCL */
  co_thread_cleanup();
}


void LpelTaskAdd(task_t *t)
{
  int to_worker;
  workerdata_t *wd;

  /* increase num of tasks in the lpel system*/
  atomic_inc(&task_count_global);

  /*TODO placement module */
  to_worker = t->uid % num_workers;
  t->owner = to_worker;

  /* place in init queue */
  wd = &workerdata[to_worker];
  pthread_mutex_lock( &wd->mtx_queue_init );
  TaskqueueAppend( &wd->queue_init, t );
  pthread_mutex_unlock( &wd->mtx_queue_init );
}

void LpelTaskRemove(task_t *t)
{
  atomic_dec(&task_count_global);
}

