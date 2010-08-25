/**
 * Main LPEL module
 *
 */

#define _GNU_SOURCE

#include <malloc.h>
#include <assert.h>

#include <sched.h>

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
/* cpuset for others-threads */
cpu_set_t cpuset_others;

/*
 * Global task count, i.e. number of tasks in the LPEL.
 * Implemented as atomic unsigned long type.
 */
atomic_t lpel_task_count = ATOMIC_INIT(0);


static pthread_barrier_t bar_worker_init;


static void LpelWorker(int id, monitoring_t *mon);


int LpelNumWorkers(void)
{
  return config->num_workers;
}


/**
 * Worker thread code
 */
static void *LpelWorkerStartup(void *arg)
{
  int id = *((int *)arg);
  monitoring_t mon_info;

  /* Init libPCL */
  co_thread_init();

  /* initialise monitoring */
  MonitoringInit(&mon_info, id, MONITORING_ALL);

  MonitoringDebug(&mon_info, "worker %d started\n", id);


  /* set affinity to id % config->proc_workers */
  if ( CpuAssignToCore(id % config->proc_workers) ) {
    MonitoringDebug(&mon_info, "worker %d assigned to core\n", id);
  }
  
  /* set worker as realtime thread */
  if ( (config->flags & LPEL_FLAG_REALTIME) != 0 ) {
    CpuAssignSetRealtime( 1 );
  }
  
  /* barrier for all worker threads, assures that the workers
    are assigned to their CPUs */
  pthread_barrier_wait( &bar_worker_init );

  /**
   * Start the worker here
   */
  LpelWorker( id, &mon_info );

  /* cleanup monitoring */ 
  MonitoringCleanup(&mon_info);

  /* Cleanup libPCL */
  co_thread_cleanup();
  
  /* exit thread */
  return NULL;
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

  /* create the cpu_set for other threads */
  {
    int  i;


    CPU_ZERO( &cpuset_others );
    if (cfg->proc_others == 0) {
      /* distribute on the workers */
      for (i=0; i<cfg->proc_workers; i++) {
        CPU_SET(i, &cpuset_others);
      }
    } else {
      /* set to proc_others */
      for( i=cfg->proc_workers;
          i<cfg->proc_workers+cfg->proc_others;
          i++ ) {
        CPU_SET(i, &cpuset_others);
      }
    }
  }

  /* store a reference to the cfg */
  config = cfg;

  /* initialise scheduler */
  SchedInit(config->num_workers, NULL);

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
  int nw = config->num_workers;
  int wid[nw];

  /* initialise barrier */
  res = pthread_barrier_init(
      &bar_worker_init,
      NULL,
      nw
      );
  assert(res == 0 );

  /* launch worker threads */
  thids = (pthread_t *) malloc(nw * sizeof(pthread_t));
  for (i = 0; i < nw; i++) {
    wid[i] = i;
    res = pthread_create(&thids[i], NULL, LpelWorkerStartup, &wid[i]);
    if (res != 0) {
      assert(0);
      /*TODO error
      perror("creating worker threads");
      exit(-1);
      */
    }
  }

  /* join on finish */
  for (i = 0; i < nw; i++) {
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





static void LpelWorker(int id, monitoring_t *mon)
{
  unsigned int loop;
  schedctx_t *sc = SchedGetContext( id );
  
  /* MAIN SCHEDULER LOOP */
  loop=0;
  do {
    task_t *t;
  
    assert( sc != NULL );


    /* select a task from the ready queue (sched) */
    t = SchedFetchNextReady( sc );
    if (t != NULL) {
      /* EXECUTE TASK (context switch) */
      t->cnt_dispatch++;
      TIMESTAMP(&t->times.start);
      t->state = TASK_RUNNING;
      co_call(t->ctx);
      TIMESTAMP(&t->times.stop);
      /* task returns in every case with a different state */
      assert( t->state != TASK_RUNNING);

      /* output accounting info */
      MonitoringPrint(mon, t);
      MonitoringDebug(mon, "worker %d, loop %u\n", id, loop);

      SchedReschedule(sc, t);
    } /* end if executed ready task */


    loop++;
  } while ( atomic_read(&lpel_task_count) > 0 );
  /* stop only if there are no more tasks in the system */
  /* MAIN SCHEDULER LOOP END */
}



void LpelThreadCreate( pthread_t *pt, void *(*start_routine)(void *), void *arg)
{
  int res;

  /* create thread with default attributes */
  res = pthread_create(pt, NULL, start_routine, arg);
  assert( res==0 );

  /* set the affinity mask */
  res = pthread_getaffinity_np( *pt, sizeof(cpu_set_t), &cpuset_others);
  assert( res==0 );
}



void LpelThreadJoin( pthread_t pt, void **joinarg)
{
  int res;
  res = pthread_join(pt, joinarg);
  assert( res==0 );
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
  atomic_inc(&lpel_task_count);

  /*TODO placement module */
  to_worker = t->uid % config->num_workers;
  t->owner = to_worker;

  /* assign to appropriate sched context */
  SchedAddTaskGlobal( t );
}

void LpelTaskRemove(task_t *t)
{
  atomic_dec(&lpel_task_count);
}

