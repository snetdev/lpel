
#include <stdlib.h>

//FIXME check availability
#include <semaphore.h>

#include <lpel.h>

#include "worlds.h"
#include "arch/atomic.h"
#include "worker.h"


/****************************************************************************/

/* internal type definitions */


typedef struct {
  sem_t sema;
} world_worker_t;


typedef struct {
  int worker_id;     /* requesting worker */
  lpel_worldfunc_t func;
  void *arg;
  lpel_task_t *task; /* requesting task */
} worldreq_t;



/****************************************************************************/

/* internal data and variables */


static int num_workers = -1;

/** worker specific data */
static world_worker_t *worker_data;


/**
 * array for the request queue
 * note that a fixed queue size of num_workers is sufficient
 */
static worldreq_t *req_queue = NULL;

static atomic_t qhead = ATOMIC_INIT(0);
static atomic_t qtail = ATOMIC_INIT(0);



/****************************************************************************/

int LpelWorldsInit(int numworkers)
{
  int i;
  assert(numworkers > 0);
  num_workers = numworkers;

  req_queue = malloc(num_workers * sizeof(worldreq_t));
  for (i=0; i<num_workers; i++) {
    req_queue[i].worker_id = -1; /* initialize to invalid worker id */
  }


  worker_data = malloc(num_workers * sizeof(world_worker_t));
  for (i=0; i<num_workers; i++) {
    world_worker_t *wd = &worker_data[i];
    sem_init(&wd->sema, 0, 0);
  }

  return 0;
}

void LpelWorldsCleanup(void)
{
  int i;
  for (i=0; i<num_workers; i++) {
    world_worker_t *wd = &worker_data[i];
    sem_destroy(&wd->sema);
  }
  free(req_queue);
  free(worker_data);
}

void LpelWorldsHandleRequest(int worker_id)
{
  worldreq_t curreq;
  world_worker_t *master_data, *self_data;
  int i, head;

  assert(worker_id >= 0);

  head = atomic_read(&qhead);
  /* local copy of current request */
  curreq = req_queue[head];
  assert(curreq.worker_id != -1);

  master_data = &worker_data[curreq.worker_id];
  self_data   = &worker_data[worker_id];

  /*
   * "Start-Barrier"
   */
  if (curreq.worker_id == worker_id) {
    /* we are the "master" thread */

    /* wait until other threads have entered the world */
    for (i=0; i<num_workers-1; i++) {
      sem_wait(&master_data->sema);
    }

    /* now master is the sole thread operating on the queue */
    { /* CRITICAL SECTION */
      int tail;
      /* invalidate the head (current) in req_queue */
      req_queue[head].worker_id = -1;
      /* move head */
      head = (head+1) % num_workers;
      atomic_set(&qhead, head);
      /* correct tail */
      tail = atomic_read(&qtail);
      if (tail >= num_workers) tail %= num_workers;
      atomic_set(&qtail, tail);
    } /* CRITICAL SECTION */

    /* signal the other threads */
    for (i=0; i<num_workers; i++) {
      if (i != worker_id) {
        sem_post(&worker_data[i].sema);
      }
    }
  } else {
    /* we are NOT master, signal master, then wait */
    sem_post(&master_data->sema);
    sem_wait(&self_data->sema);
  }

  /**********************************/
  /* EXECUTE THE REQUESTED FUNCTION */
  curreq.func(curreq.arg);
  /**********************************/

  /*
   * "Stop-Barrier"
   */
  if (curreq.worker_id == worker_id) {
    /* we are the "master" thread */
    /* wait until other threads have left the world */
    for (i=0; i<num_workers-1; i++) {
      sem_wait(&master_data->sema);
    }

    /* now we can wakeup the task */
    LpelWorkerTaskWakeupLocal( curreq.task->worker_context, curreq.task);
  } else {
    /* we are NOT master, signal master before continuing */
    sem_post(&master_data->sema);
  }

}


void LpelWorldsRequest(int worker_id, lpel_worldfunc_t fun, void *arg,
    lpel_task_t *task)
{
  int tail;
  worldreq_t *req;
  workermsg_t msg;
  assert(worker_id >= 0);

  /* append */
  tail = fetch_and_inc(&qtail) % num_workers;
  req = &req_queue[tail];
  req->worker_id = worker_id;
  req->func      = fun;
  req->arg       = arg;
  req->task      = task;

  /* broadcast message */
  msg.type = WORKER_MSG_WORLDREQ;
  msg.body.from_worker = worker_id;
  LpelWorkerBroadcast(&msg);
}

