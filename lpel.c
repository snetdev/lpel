/**
 * Main LPEL module
 *
 */

#define _GNU_SOURCE

#include <malloc.h>
#include <assert.h>

#include <sched.h>
#include <unistd.h>  /* sysconf() */
#include <sys/types.h> /* pid_t */
#include <linux/unistd.h>
#include <sys/syscall.h> 

#include <pthread.h> /* worker threads are pthreads (linux has 1-1 mapping) */
#include <pcl.h>     /* tasks are executed in user-space with help of
                        GNU Portable Coroutine Library  */

#include "lpel.h"

/*!! link with -lcap */
#ifdef LPEL_USE_CAPABILITIES
#  include <sys/capability.h>
#endif

/* macro using syscall for gettid, as glibc doesn't provide a wrapper */
#define gettid() syscall( __NR_gettid )


#include "task.h"
#include "scheduler.h"



struct lpelthread {
  pthread_t pthread;
  void (*func)(void *);
  void *arg;
};

typedef struct {
  pthread_t     thread;
  worker_ctx_t *context;
} worker_thread_t;

static worker_thread_t *worker_thread_data;

/* Keep copy of the (checked) configuration provided at LpelInit() */
static lpelconfig_t config;

/* cpuset for others-threads */
static cpu_set_t cpuset_others;



static int CanSetRealtime(void)
{
#ifdef LPEL_USE_CAPABILITIES
  cap_t caps;
  cap_flag_value_t cap;
  /* obtain caps of process */
  caps = cap_get_proc();
  if (caps == NULL) {
    /*TODO error msg*/
    return 0;
  }
  cap_get_flag(caps, CAP_SYS_NICE, CAP_EFFECTIVE, &cap);
  return (cap == CAP_SET);
#else
  return 0;
#endif
}



/**
 * Get the number of workers
 */
int LpelNumWorkers(void)
{
  return config.num_workers;
}



/**
 * Assigns worker with specified id to a core
 * @return 0 if successful, errno otherwise
 */
int LpelAssignWorkerToCore( int wid, int *core)
{
  int res;
  pid_t tid;
  cpu_set_t cpuset; 
  
  *core = wid % config.proc_workers;
  
  CPU_ZERO(&cpuset);
  CPU_SET( *core, &cpuset);

  /* get thread id */
  tid = gettid();
  
  res = sched_setaffinity(tid, sizeof(cpu_set_t), &cpuset);
  return (res == 0) ? 0 : errno;
}

int LpelAssignWorkerRealtime( void)
{
  int res = 0;
  if ( (config.flags & LPEL_FLAG_REALTIME) != 0 ) {
    pid_t tid;
    struct sched_param param;

    /* get thread id */
    tid = gettid();

    param.sched_priority = 1; /* lowest real-time, TODO other? */
    res = sched_setscheduler(tid, SCHED_FIFO, &param);
    return (res == 0) ? 0 : errno;
  }
  return -1;
}

/**
 * Startup the worker
 */
static void *WorkerStartup(void *arg)
{
  int wid = *((int *)arg);
  int res, core;
  monitoring_t mon_info;

  /* Init libPCL */
  co_thread_init();

  /* initialise monitoring */
  MonitoringInit(&mon_info, wid, MONITORING_ALL);

  MonitoringDebug(&mon_info, "worker %d started\n", wid);


  /* set affinity to id % config->proc_workers */
  res = AssignWorkerToCore( wid, &core);
  if ( res == 0) {
    MonitoringDebug(&mon_info, "worker %d assigned to core %d\n", wid, core);
  } else {
    /*TODO warning, check errno=res */
  }
  
  /* set worker as realtime thread, if set so */
    res = AssignWorkerRealtime( wid);
    if (res == 0) {
      MonitoringDebug(&mon_info, "worker %d set realtime priority\n", wid);
    } else {
      /*TODO warning, check errno? */
    }
  }
  

  /**************************************************************************/
  /* Start the scheduler task */
  SchedTask( wid, &mon_info );
  /* scheduler task will return when there is no more work to do */
  /**************************************************************************/

  MonitoringDebug(&mon_info, "worker %d exiting\n", wid);

  /* cleanup monitoring */ 
  MonitoringCleanup(&mon_info);

  /* Cleanup libPCL */
  co_thread_cleanup();
  
  /* exit thread */
  pthread_exit(NULL);
}




/* test only for a single flag in CheckConfig */
#define LPEL_ICFG(f)   (( cfg->flags & (f) ) != 0 )

static void CheckConfig( lpelconfig_t *cfg)
{
  int proc_avail;

  /* query the number of CPUs */
  proc_avail = sysconf(_SC_NPROCESSORS_ONLN);
  if (proc_avail == -1) {
    /*TODO _SC_NPROCESSORS_ONLN not available! */
  }

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
    /* check for privileges needed for setting realtime */
    if ( CanSetRealtime()==0 ) {
      /*TODO warning */
      /* clear flag */
      cfg->flags &= ~(LPEL_FLAG_REALTIME);
    }
  }
}


static void CreateCpusetOthers( lpelconfig_t *cfg)
{
  int  i;
  /* create the cpu_set for other threads */
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
 * otherwise, following sanity checks are performed:
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
  int i;

  /* store a local copy of cfg */
  config = *cfg;
  
  /* check (and correct) the config */
  CheckConfig( &config);
  /* create the cpu affinity set for non-worker threads */
  CreateCpusetOthers( &config);

  /* Init libPCL */
  co_thread_init();
  
  /* initialise scheduler */
  SchedInit( config.num_workers, NULL);

  /* create table for worker data structures */
  worker_thread_data = (worker_thread_t *) malloc( config.num_workers * sizeof(worker_thread_t) );

  /* for each worker id, create data structure and store in table */
  for( i=0; i<config.num_workers; i++) {
    worker_thread_data[i].context = SchedWorkerDataCreate(i);
    worker_thread_data[i].thread = NULL;
  }
}

/**
 * Create and execute the worker threads
 * 
 */
void LpelWorkerStart(void)
{
  int i, res;

  for( i=0; i<config.num_workers; i++) {
    res = pthread_create(
        &worker_thread_data[i].thread,  /* pthread_t is kept for joining */
        NULL,
        WorkerStartup,    
        (void *) &worker_thread_data[i].context  /* context passed to worker */
        );
    /*TODO error*/
    assert( res == 0);
  }
}


void LpelWorkerStop(void)
{
  int i, res;
  
  for( i=0; i<config.num_workers; i++) {
    //TODO signal termination to workers
    // SchedWorkersTerminate() ?
  }

  /* wait on the workers */
  for( i=0; i<config.num_workers; i++) {
    res = pthread_join( worker_thread_data[i].thread, NULL);
    /*TODO error*/
    assert( res == 0);
  }

}

/**
 * Cleans the LPEL up
 * - free the data structures of worker threads
 */
void LpelCleanup(void)
{

  /* for each worker, destroy its context */
  for( i=0; i<config.num_workers; i++) {
    SchedWorkerDataDestroy( worker_thread_data[i].context);
  }

  /* destroy worker data table */
  free( worker_thread_data);

  /* Cleanup scheduler */
  SchedCleanup();

  /* Cleanup libPCL */
  co_thread_cleanup();
}



static void *ThreadStartup( void *arg)
{
  lpelthread_t *td = (lpelthread_t *)arg;

  /* set the cpu set */
  //TODO
  res = pthread_getaffinity_np( lt->pthread, sizeof(cpu_set_t), &cpuset_others);
  assert( res==0 );

  /* call the function */
  td->func( td->arg);

}



void LpelThreadCreate( lpelthread_t *thread,
    void *(*func)(void *), void *arg)
{
  int res;

  thread->func = func;
  thread->arg = arg;

  /* create thread with default attributes */
  res = pthread_create( &thread->pthread, NULL, ThreadStartup, thread);
  assert( res==0 );
}



void LpelThreadJoin( lpelthread_t *lt)
{
  int res;
  res = pthread_join(lt->pthread, NULL);
  assert( res==0 );
}


/**
 *
 * Has to be thread-safe.
 */
void LpelTaskToWorker(task_t *t)
{
  int to_worker;

  /*TODO placement module */
  to_worker = t->uid % config.num_workers;
  t->owner = to_worker;

  /* assign to appropriate sched context */
  SchedAssignTask( to_worker, t );
}

