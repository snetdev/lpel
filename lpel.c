/**
 * Main LPEL module
 * contains:
 * - startup and shutdown routines
 * - worker thread code
 *
 */

#include <malloc.h>
#include <assert.h>

#include <pthread.h> /* worker threads are pthreads (linux has 1-1 mapping) */
#include <pcl.h>     /* tasks are executed in user-space with help of
                        GNU Portable Coroutine Library  */

#include "lpel.h"

#include "task.h"
#include "stream.h"
#include "scheduler.h"

/* used (imported) modules of LPEL */
#include "cpuassign.h"
#include "timing.h"
#include "monitoring.h"
#include "atomic.h"



/* Keep pointer to the configuration provided at LpelInit() */
static lpelconfig_t *config;

/*
 * Global task count, i.e. number of tasks in the LPEL.
 * Implemented as atomic unsigned long type.
 */
static atomic_t task_count_global = ATOMIC_INIT(0);


static pthread_barrier_t bar_worker_init;


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
  monitoring_t mon_info;

  /* Init libPCL */
  co_thread_init();
  

  /* initialise monitoring */
  MonitoringInit(&mon_info, id, MONITORING_ALL);

  MonitoringDebug(&mon_info, "worker %d started\n", id);

  /* set affinity to id % config->proc_workers */
  if (b_assigncore) {
    if ( CpuAssignToCore(id) ) {
      MonitoringDebug(&mon_info, "worker %d assigned to core\n", id);
    }
  }
  
  /* barrier for all worker threads, assures that the workers
    are assigned to their CPUs */
  pthread_barrier_wait( &bar_worker_init );

  /**
   * Start the worker here
   */
  /* MAIN SCHEDULER LOOP */
  loop=0;
  do {
    task_t *t;
  
    assert( sc != NULL );

    CpuAssignSetPreemptable(false);

    /* select a task from the ready queue (sched) */
    t = SchedFetchNextReady( sc );
    if (t != NULL) {
      /* EXECUTE TASK (context switch) */
      t->cnt_dispatch++;
      TIMESTAMP(&t->times.start);
      t->state = TASK_RUNNING;
      co_call(t->ctx);
      TIMESTAMP(&t->times.stop);
      /* task returns in every case with a different state than RUNNING */

      /* output accounting info */
      MonitoringPrint(&mon_info, t);
      MonitoringDebug(&mon_info, "worker %d, loop %u\n", id, loop);

      SchedReschedule(sc, t);
    } /* end if executed ready task */

    CpuAssignSetPreemptable(true);

    loop++;
  } while ( atomic_read(&task_count_global) > 0 );
  /* stop only if there are no more tasks in the system */
  /* MAIN SCHEDULER LOOP END */
    
  MonitoringCleanup(&mon_info);

  /* Cleanup libPCL */
  co_thread_cleanup();
  
  /* exit thread */
  pthread_exit(NULL);
}




/* test only for a single flag in LpelInit! */
#define LPEL_ICFG(f)   (( cfg->flags & (f) ) != 0 )


/**
 * Initialise the LPEL
 *
 * If FLAG_AUTO is set, values set for num_workers, proc_workers and
 * proc_others are ignored and set for default as follows:
 *
 * AUTO: if #proc_avail > 1:
 *         num_workers = proc_workers = #proc_avail - 1
 *         proc_others = 1
 *       else
 *         proc_workers = num_workers = 1,
 *         proc_others = 0
 *
 * otherwise, followin sanity checks are performed:
 *
 *  num_workers, proc_workers > 0
 *  proc_others >= 0
 *  num_workers = i*proc_workers, i>0
 *
 * If the realtime flag is invalid or the process has not
 * the appropriate privileges, it is ignored and a warning
 * will be displayed
 *
 * REALTIME: only valid, if
 *       #proc_avail >= proc_workers + proc_others &&
 *       proc_others != 0 &&
 *       num_workers == proc_workers 
 *
 */
void LpelInit(lpelconfig_t *cfg)
{
  int proc_avail;

  /* determine number of cpus */
  proc_avail = CpuAssignQueryNumCpus();

  if ( LPEL_ICFG( LPEL_FLAG_AUTO ) ) {

    /* default values */
    if (proc_avail > 1) {
      /* multiprocessor */
      cfg->num_workers = cfg->proc_workers = (proc_avail-1);
      cfg->proc_others = 1;
    } else {
      /* uniprocessor */
      cfg->num_workers = cfg->proc_workers = 1;
      cfg->proc_others = 0;
    }
  } else {

    /* sanity checks */
    if ( cfg->num_workers <= 0 ||  cfg->proc_workers <= 0 ) {
      /* TODO error */
      assert(0);
    }
    if ( cfg->proc_others < 0 ) {
      /* TODO error */
      assert(0);
    }
    if ( (cfg->num_workers % cfg->proc_workers) != 0 ) {
      /* TODO error */
      assert(0);
    }
  }


  /* check realtime flag sanity */
  if ( LPEL_ICFG( LPEL_FLAG_REALTIME ) ) {
    /* check for validity */
    if (
        (proc_avail < cfg->proc_workers + cfg->proc_others) ||
        ( cfg->proc_others == 0 ) ||
        ( cfg->num_workers != cfg->proc_workers )
       ) {
      /*TODO warning */
      /* clear flag */
      cfg->flags &= ~(LPEL_FLAG_REALTIME);
    }
  }

  
  /* store a reference to the cfg */
  config = cfg;

  /* initialise scheduler */
  SchedInit(num_workers, NULL);

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

  /* initialise barrier */
  res = pthread_barrier_init(
      &bar_worker_init,
      NULL,
      num_workers
      );
  assert(res == 0 );

  /* launch worker threads */
  thids = (pthread_t *) malloc(num_workers * sizeof(pthread_t));
  for (i = 0; i < num_workers; i++) {
    wid[i] = i;
    res = pthread_create(&thids[i], NULL, LpelWorker, &wid[i]);
    if (res != 0) {
      assert(0);
      /*TODO error
      perror("creating worker threads");
      exit(-1);
      */
    }
  }

  /* join on finish */
  for (i = 0; i < num_workers; i++) {
    pthread_join(thids[i], NULL);
  }

  /* destroy barrier */
  res = pthread_barrier_destroy(&bar_worker_init);
  assert( res == 0 );
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

/**
 * This function is currently called from TaskCreate
 * (this might change)
 *
 * Has to be thread-safe.
 */
void LpelTaskAdd(task_t *t)
{
  int to_worker;

  /* increase num of tasks in the lpel system*/
  atomic_inc(&task_count_global);

  /*TODO placement module */
  to_worker = t->uid % num_workers;
  t->owner = to_worker;

  /* assign to appropriate sched context */
  SchedAddTaskGlobal( t );
}

void LpelTaskRemove(task_t *t)
{
  atomic_dec(&task_count_global);
}

