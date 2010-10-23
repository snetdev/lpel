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

#include <pthread.h> /* worker threads are OS threads */
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
  bool detached;
  int worker;
  void (*func)(struct lpelthread *, void *);
  void *arg;
  char[20] name;
  monitoring_t mon;
};

/* table for storing workers thread data */
static lpelthread_t *worker_thread_data;

/* Keep copy of the (checked) configuration provided at LpelInit() */
static lpelconfig_t config;

/* cpuset for others-threads */
static cpu_set_t cpuset_others;


static void CleanupEnv( lpelthread_t *env)
{
  //TODO
  free( env);
}

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

static void ThreadAssign( lpelthread_t *env)
{
  int res;
  pid_t tid;

  /* get thread id */
  tid = gettid();
  
  if ( env->worker == -1) {
    /* a non worker (others) thread */
    res = sched_setaffinity(tid, sizeof(cpu_set_t), &cpuset_others);
    assert( res == 0);

  } else {
    /* a worker thread */
    int core;
    cpu_set_t cpuset;

    assert( (env->worker >= 0) && (env->worker < config.num_workers) );
    
    core = wid % config.proc_workers;

    CPU_ZERO(&cpuset);
    CPU_SET( core, &cpuset);
    res = sched_setaffinity(tid, sizeof(cpu_set_t), &cpuset);
    assert( res == 0);

    /* make non-preemptible */
    if ( (config.flags & LPEL_FLAG_REALTIME) != 0 ) {
      struct sched_param param;
      param.sched_priority = 1; /* lowest real-time, TODO other? */
      res = sched_setscheduler(tid, SCHED_FIFO, &param);
      assert( res == 0);
    }
}
#if 0
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

  MonitoringDebug(&mon_info, "worker %d exiting\n", wid);
  /* cleanup monitoring */ 
  MonitoringCleanup(&mon_info);
#endif



/**
 * Startup a lpel thread
 */
static void *ThreadStartup( void *arg)
{
  lpelthread_t *env = (lpelenv_t *)arg;

  //env.mon

  /* Init libPCL */
  co_thread_init();

  /* assign to cpu(s) */
  ThreadAssign( env);

  /* call the function */
  env->func( env, env->arg);

  /* if detached, cleanup the env now,
     otherwise it will be done on join */
  if (env->detached) {
    /* workers are never detached */
    assert( env->worker == -1);
    CleanupEnv( env);
  }
  
  /* Cleanup libPCL */
  co_thread_cleanup();

  return NULL;
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
  worker_thread_data = (lpelthread_t *) malloc(
      config.num_workers * sizeof(lpelthread_t)
      );

  /* for each worker, create data structure and 
     spawn worker */
  for( i=0; i<config.num_workers; i++) {
    
    lpelthread_t *env = &worker_thread_data[i];
    /* for convenience */
    env->pthread = NULL;
    env->func = SchedTask; //TODO
    env->detached = false;
    /* relevant data */
    env->worker = i;
    snprintf( &env->name, 21, "worker%02d", i);
    env->arg = SchedWorkerDataCreate(i);
    //TODO mon init

    /* spawn thread */
    res = pthread_create(
        &env->pthread, /* pthread_t is kept for joining */
        NULL,          /* default attributes (joinable) */
        ThreadStartup,    
        (void *) env 
        );
    /*TODO error*/
    assert( res == 0);
  }
}



void LpelStopWorker(void)
{
  int i, res;
  
  /* signal termination to workers */
  for( i=0; i<config.num_workers; i++) {
    //TODO SchedWorkersTerminate() ?
  }


}

/**
 * Cleans the LPEL up
 * - wait for the workers to finish
 * - free the data structures of worker threads
 */
void LpelCleanup(void)
{

  /* wait on the workers */
  for( i=0; i<config.num_workers; i++) {
    lpelthread_t *env = &worker_thread_data[i];
    /* wait for the worker to finish */
    res = pthread_join( env->pthread, NULL);
    /*TODO error*/
    assert( res == 0);

    /* delete worker context */
    SchedWorkerDataDestroy( env->arg);
    
    CleanupEnv( env);
  }

  /* destroy worker data table */
  free( worker_thread_data);

  /* Cleanup scheduler */
  SchedCleanup();

  /* Cleanup libPCL */
  co_thread_cleanup();
}





lpelthread_t *LpelThreadCreate( void (*func)(lpelthread_t *, void *),
    void *arg, bool detached, char *name)
{
  int res;
  pthread_attr_t attr;

  lpelthread_t *env = (lpelthread_t *) malloc( sizeof( lpelthread_t));

  env->worker = -1;
  env->func = func;
  env->arg = arg;
  env->detached = detached;
  if (name != NULL) {
    strncpy( &env->name, name, 21);
    env->name[20]= '\0';
  } else {
    memset( &env->name, '\0', 21);
  }

  //TODO monitoring

  /* create attributes for joinable/detached*/
  pthread_attr_init( &attr);
  pthread_attr_setdetachstate( &attr,
      (detached) ? PTHREAD_CREATE_DETACHED : PTHREAD_CREATE_JOINABLE
      );
  
  res = pthread_create( &env->pthread, &attr, ThreadStartup, env);
  assert( res==0 );

  pthread_attr_destroy( &attr);

  return env;
}


/**
 * @pre  thread must not have been created detached
 * @post lt is freed and the reference is invalidated
 */
void LpelThreadJoin( lpelthread_t *env)
{
  int res;
  assert( env->detached == false);
  res = pthread_join(env->pthread, NULL);
  assert( res==0 );

  /* cleanup */
  CleanupEnv( env);
}


/**
 * Get the number of workers
 */
int LpelNumWorkers(void)
{
  return config.num_workers;
}


/**
 *
 * Has to be thread-safe.
 */
void LpelTaskToWorker( task_t *t)
{
  int to_worker;

  /*TODO placement module */
  to_worker = t->uid % config.num_workers;
  t->owner = to_worker;

  /* assign to appropriate sched context */
  SchedAssignTask( to_worker, t );
}

