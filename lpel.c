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
#include "set.h"
#include "stream.h"
#include "streamtable.h"
#include "debug.h"
#include "scheduler.h"

/* used (imported) modules of LPEL */
#include "cpuassign.h"
#include "timing.h"
#include "monitoring.h"
#include "atomic.h"




/*
 * Global task count, i.e. number of tasks in the LPEL.
 * Implemented as atomic unsigned long type.
 */
static atomic_t task_count_global = ATOMIC_INIT(0);

static int num_workers = -1;
static bool b_assigncore = false;


#define EXPAVG_ALPHA  0.1f


#define BIT_IS_SET(vec,b)   (( (vec) & (b) ) == (b) )

int LpelNumWorkers(void)
{
  return num_workers;
}


/**
 * Worker thread code
 */
static void *LpelWorker(void *arg)
{
  unsigned int loop;
  int id = *((int *)arg);
  schedctx_t *sc = SchedGetContext( id );
  monitoring_t *mon_info;

  /* Init libPCL */
  co_thread_init();
  

  /* initialise monitoring */
  MonitoringInit(&mon_info, id);

  MonitoringDebug(mon_info, "worker %d started\n", id);

  /* set affinity to id=CPU */
  if (b_assigncore) {
    if ( CpuAssignToCore(id) ) {
      MonitoringDebug(mon_info, "worker %d assigned to core\n", id);
    }
  }
  


  /* MAIN SCHEDULER LOOP */
  loop=0;
  do {
    task_t *t;
    timing_t ts;
  
    assert( sc != NULL );

    /* select a task from the ready queue (sched) */
    t = SchedFetchNextReady( sc );

    if (t != NULL) {

      CpuAssignSetPreemptable(false);

      /* start timing (mon) */
      TimingStart(&ts);

      /* EXECUTE TASK (context switch) */
      t->cnt_dispatch++;
      t->state = TASK_RUNNING;
      co_call(t->ctx);
      /* task returns in every case with a different state than RUNNING */
      

      /* end timing (mon) */
      TimingEnd(&ts);
      TimingSet(&t->time_lastrun, &ts);
      TimingAdd(&t->time_totalrun, &ts);
      TimingExpAvg(&t->time_expavg, &ts, EXPAVG_ALPHA);

      CpuAssignSetPreemptable(true);

      /* output accounting info (mon) */
      MonitoringPrint(mon_info, t);
      MonitoringDebug(mon_info, "worker %d, loop %u\n", id, loop);

      SchedReschedule(sc, t);
    } /* end if executed ready task */


    loop++;
  } while ( atomic_read(&task_count_global) > 0 );
  /* stop only if there are no more tasks in the system */
  /* MAIN SCHEDULER LOOP END */
    
  MonitoringCleanup(mon_info);

  /* Cleanup libPCL */
  co_thread_cleanup();
  
  /* exit thread */
  pthread_exit(NULL);
}






/**
 * Initialise the LPEL
 * - if num_workers==-1, determine the number of worker threads automatically
 */
void LpelInit(lpelconfig_t *cfg)
{
  int cpus;

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
    // TODO remove next line
    b_assigncore = true;
  }

  /* initialise scheduler */
  SchedInitialise(num_workers, NULL);

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
  int wid[num_workers];


  // launch worker threads
  thids = (pthread_t *) malloc(num_workers * sizeof(pthread_t));
  for (i = 0; i < num_workers; i++) {
    wid[i] = i;
    res = pthread_create(&thids[i], NULL, LpelWorker, &wid[i]);
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

}


/**
 * Cleans the LPEL up
 * - free the data structures of worker threads
 */
void LpelCleanup(void)
{
  SchedCleanup();

  /* Cleanup libPCL */
  co_thread_cleanup();
}


void LpelTaskAdd(task_t *t)
{
  int to_worker;

  /* increase num of tasks in the lpel system*/
  atomic_inc(&task_count_global);

  /*TODO placement module */
  to_worker = t->uid % num_workers;
  t->owner = to_worker;

  /* place in init queue */
  SchedAddTaskGlobal( t );
}

void LpelTaskRemove(task_t *t)
{
  atomic_dec(&task_count_global);
}

