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

#include "cpuassign.h"




typedef struct {
  /* ID */
  unsigned int id;
  /* init queue */
  /* ready queue(s) */
  void *readyQ;
  /* waiting queue */
  /* current task */
  /* monitoring output info */
} workerdata_t;

static workerdata_t *workerdata = NULL;

static int num_workers = -1;

static pthread_key_t worker_id_key;






/**
 * Worker thread code
 */
static void *LpelWorker(void *idptr)
{
  /* data contains id, set as thread specific worker_id */
  pthread_setspecific(worker_id_key, idptr);

  /* set affinity to id=CPU */

  /* set scheduling policy */


  /* MAIN LOOP */
  /* fetch new tasks from InitQ, insert into ReadyQ (sched) */
  /* select a task from the ReadyQ (sched) */
  /* set current_task */
  /* start timing (mon) */
  /* context switch */
  /* end timing (mon) */
  /* output accounting info (mon) */
  /* check state of task, place into appropriate queue */
  /* iterate through waiting queue, check r/w events */
  /* (iterate through nap queue, check alert-time) */
  /* MAIN LOOP END */
  
  /* exit thread */
}






/**
 * Initialise the LPEL
 * - if num_workers==-1, determine the number of worker threads automatically
 * - create the data structures for each worker thread
 * - create necessary TSD
 */
void LpelInit(const int nworkers)
{
  if (nworkers==-1) {
    /* one available core has to be reserved for IO tasks
       and other system threads */
    num_workers = CpuAssignQueryNumCpus() - 1;
  } else {
    num_workers = nworkers;
  }
  /* TODO: exclusive SCHED_FIFO possible? */
  if (num_workers < 1) num_workers = 1;


  /* Create the data structures */
  workerdata = (workerdata_t *) malloc( num_workers*sizeof(workerdata_t) );

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
 * - free the TSD
 */
void LpelCleanup(void)
{
  free(workerdata);
}


