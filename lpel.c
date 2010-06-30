/**
 * Main LPEL module
 * contains:
 * - startup and shutdown routines
 * - worker thread code
 * - (stream and task management)
 *
 *
 */

#include <malloc.h>

#include <pthread.h> /* worker threads are pthreads (linux has 1-1 mapping) */
#include <pcl.h>     /* tasks are executed in user-space with help of
                        GNU Portable Coroutine Library  */

#include "lpel.h" /* private header also includes lpel.h*/

#include "task.h"
#include "taskqueue.h"
#include "set.h"
#include "stream.h"


/* used (imported) modules of LPEL */
#include "cpuassign.h"
#include "timing.h"
#include "scheduler.h"
#include "atomicop.h"


typedef struct {
  unsigned int id;           /* worker ID */
  taskqueue_t queue_init;    /* init queue */
  pthread_mutex_t mtx_queue_init;
  void       *queue_ready;   /* ready queue */
  taskqueue_t queue_waiting; /* waiting queue */
  task_t *current_task;      /* current task */
  /*TODO monitoring output info */
} workerdata_t;

/* array of workerdata_t, one for each worker */
static workerdata_t *workerdata = NULL;

/*
 * Global task count, i.e. number of tasks in the LPEL.
 * Implemented as atomic unsigned long type.
 */
static aulong_t task_count_global = AULONG_INIT(0);

static int num_workers = -1;
static bool b_assigncore = false;

static pthread_key_t worker_id_key;


#define EXPAVG_ALPHA  0.5f

#define TSD_WORKER_ID (*((int *)pthread_getspecific(worker_id_key)))


/*
 * Get current worker id
 */
int LpelGetWorkerId(void)
{
  return TSD_WORKER_ID;
}

/*
 * Get current executed task
 */
task_t *LpelGetCurrentTask(void)
{
  return workerdata[TSD_WORKER_ID].current_task;
}


void LpelTaskcntInc(void)
{
  aulong_inc(&task_count_global);
}

void LpelTaskcntDec(void)
{
  aulong_dec(&task_count_global);
}


static bool WaitingTest(task_t *wt)
{
  return *wt->event_ptr == true;
}

static void WaitingRemove(task_t *wt)
{
  wt->state = TASK_READY;
  SchedPutReady( workerdata[TSD_WORKER_ID].queue_ready, wt );
}


inline static LpelAttrIsSet(unsigned long vec, lpelconfig_attr_t b)
{
  return ((vec)&(b)==(b));
}


/**
 * Worker thread code
 */
static void *LpelWorker(void *idptr)
{
  int id = *((int *)idptr);
  /* set idptr as thread specific */
  pthread_setspecific(worker_id_key, idptr);

  /* set affinity to id=CPU */
  if (b_assigncore) CpuAssignToCore(id);

  /* set scheduling policy */
  workerdata[id].queue_ready = SchedInit();

  /* MAIN LOOP */
  while (1) {
    task_t *t;
    timing_t ts;

    /* fetch new tasks from init queue, insert into ready queue (sched) */
    pthread_mutex_lock( &workerdata[id].mtx_queue_init );
    t = TaskqueueRemove( &workerdata[id].queue_init );
    while (t != NULL) {
      assert( t->state == TASK_INIT );
      t->state = TASK_READY;
      SchedPutReady( workerdata[id].queue_ready, t );
      /* for next iteration: */
      t = TaskqueueRemove( &workerdata[id].queue_init );
    }
    pthread_mutex_unlock( &workerdata[id].mtx_queue_init );
    

    /* select a task from the ready queue (sched) */
    t = SchedFetchNextReady( workerdata[id].queue_ready );
    assert( t->state == TASK_READY );
    /* set current_task */
    workerdata[id].current_task = t;


    /* start timing (mon) */
    TimingStart(&ts);

    /* EXECUTE TASK (context switch) */
    t->cnt_dispatch++;
    t->state = TASK_RUNNING;
    co_call(t->code);
    /* task returns with a different state, except it reached the end of code */

    /* end timing (mon) */
    TimingEnd(&ts);
    TimingSet(&t->time_lastrun, &ts);
    TimingAdd(&t->time_totalrun, &ts);
    TimingExpAvg(&t->time_expavg, &ts, EXPAVG_ALPHA);


    /* check state of task, place into appropriate queue */
    switch(t->state) {
    case TASK_ZOMBIE:  /* task exited by calling TaskExit() */
    case TASK_RUNNING: /* task exited by reaching end of code! */
      TimingEnd(&t->time_alive);
      /*TODO if joinable, place into join queue, else destroy */
      TaskDestroy(t);
      break;
    case TASK_WAITING: /* task returned from a blocking call*/
      /* put into waiting queue */
      TaskqueueAppend( &workerdata[id].queue_waiting, t );
      break;
    case TASK_READY: /* task yielded execution  */
      /* put into ready queue */
      SchedPutReady( workerdata[id].queue_ready, t );
      break;
    default:
      assert(0); /* should not be reached */
    }

    /*TODO output accounting info (mon) */

    /* iterate through waiting queue, check r/w events */
    TaskqueueIterateRemove( &workerdata[id].queue_waiting,
                            WaitingTest, WaitingRemove
                            );

    /*XXX (iterate through nap queue, check alert-time) */

    /* check for exit condition */
    if (aulong_read(&task_count_global)<=0) break;
  } /* MAIN LOOP END */
  
  /* exit thread */
  pthread_exit(NULL);
}






/**
 * Initialise the LPEL
 * - if num_workers==-1, determine the number of worker threads automatically
 * - create the data structures for each worker thread
 */
void LpelInit(lpelconfig_t *cfg)
{
  int i;

  if (cfg->num_workers == -1) {
    /* one available core has to be reserved for IO tasks
       and other system threads */
    num_workers = CpuAssignQueryNumCpus() - 1;
  } else {
    num_workers = cfg->num_workers;
  }
  if (num_workers < 1) num_workers = 1;
  
  /* Exclusive assignment possible? */
  if ( LpelAttrIsSet(cfg->attr, LPEL_ATTR_ASSIGNCORE) ) {
    if ( !CpuAssignCanExclusively() ) {
      ;/*TODO emit warning, fallback */
      b_assigncore = false;
    } else {
      b_assigncore = true;
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

  /* Create thread-specific data key for worker_id */
  pthread_key_create(&worker_id_key, NULL);


  // launch worker threads
  thids = (pthread_t *) malloc(num_workers * sizeof(pthread_t));
  for (i = 0; i < num_workers; i++) {
    workerdata[i].id = i;
    res = pthread_create(&thids[i], NULL, LpelWorker, &(workerdata[i].id));
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


