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
#include "stream.h"
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

static pthread_barrier_t bar_worker_init;

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
  monitoring_t mon_info;

  /* Init libPCL */
  co_thread_init();
  

  /* initialise monitoring */
  MonitoringInit(&mon_info, id, MONITORING_ALL);

  MonitoringDebug(&mon_info, "worker %d started\n", id);

  /* set affinity to id=CPU */
  if (b_assigncore) {
    if ( CpuAssignToCore(id) ) {
      MonitoringDebug(&mon_info, "worker %d assigned to core\n", id);
    }
  }
  
  /* barrier for all worker threads, assures that the workers
    are assigned to their CPUs */
  pthread_barrier_wait( &bar_worker_init );

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
  if (res !=0 ) {
    /*TODO error */
      assert(0);
  }

  // launch worker threads
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

  // join on finish
  for (i = 0; i < num_workers; i++) {
    pthread_join(thids[i], NULL);
  }

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

