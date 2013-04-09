
#include <stdlib.h>
#include <assert.h>

//FIXME check availability
#include <semaphore.h>


#include "spmdext.h"

#include "lpelcfg.h"
#include "arch/atomic.h"
#include "decen_worker.h"


/****************************************************************************/

/* internal type definitions */



typedef struct {
  workerctx_t *wctx;     /* requesting worker */
  lpel_spmdfunc_t func;
  void *arg;
  lpel_task_t *task; /* requesting task */
} spmdreq_t;



typedef struct {
  sem_t sema;
  spmdreq_t *curreq; /* pointer to local copy of
                         the current request on the stack */
  int pending;
  int padding[7];
} spmd_worker_t;


/****************************************************************************/

/* internal data and variables */


static int num_workers = -1;

/** worker specific data */
static spmd_worker_t *worker_data = NULL;


/**
 * array for the request queue
 * note that a fixed queue size of num_workers is sufficient
 */
static spmdreq_t *req_queue = NULL;

static atomic_t qhead = ATOMIC_INIT(0);
static atomic_t qtail = ATOMIC_INIT(0);



/****************************************************************************/

static inline int GetVId(int self_id, int master_id) {
  int vid = master_id - self_id;
  return (vid < 0) ? vid+num_workers : vid;
}

/****************************************************************************/

int LpelSpmdInit(int numworkers)
{
  int i;
  assert(numworkers > 0);
  num_workers = numworkers;

  /* allocate request queue */
  req_queue = malloc(num_workers * sizeof(spmdreq_t));
  for (i=0; i<num_workers; i++) {
    req_queue[i].wctx = NULL; /* initialize to invalid worker id */
  }

  /* allocate private worker data */
  worker_data = malloc(num_workers * sizeof(spmd_worker_t));
  for (i=0; i<num_workers; i++) {
    spmd_worker_t *wd = &worker_data[i];
    sem_init(&wd->sema, 0, 0);
    wd->curreq = NULL;
    wd->pending = 0;
  }

  return 0;
}

void LpelSpmdCleanup(void)
{
  int i;
  for (i=0; i<num_workers; i++) {
    spmd_worker_t *wd = &worker_data[i];
    sem_destroy(&wd->sema);
  }
  free(req_queue);
  free(worker_data);
}

void LpelSpmdHandleRequests(int worker_id)
{
	spmdreq_t curreq;
  spmd_worker_t *master_data, *self_data;
  int i, head, cur_worker_id;

  assert(worker_id >= 0 && worker_id < num_workers);

  while (1) {
    head = atomic_read(&qhead);
    /* local copy of current request */
    curreq = req_queue[head];

    if (curreq.wctx == NULL) {
      /* all requests handled */
      break;
    };

    cur_worker_id = curreq.wctx->wid;
    assert(cur_worker_id != -1);

    master_data = &worker_data[cur_worker_id];
    self_data   = &worker_data[worker_id];

    /*
     * "Start-Barrier"
     */
    if (cur_worker_id == worker_id) {
      /* we are the "master" thread */

      /* wait until other threads have entered the spmd */
      for (i=0; i<num_workers-1; i++) {
        sem_wait(&master_data->sema);
      }

      /* now master is the sole thread operating on the queue */
      { /* CRITICAL SECTION */
        int tail;
        /* invalidate the head (current) in req_queue */
        req_queue[head].wctx = NULL;
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
    } /* End "Start-Barrier" */


    /* set the pointer to curreq */
    self_data->curreq = &curreq;

    /**********************************/
    /* EXECUTE THE REQUESTED FUNCTION */
    {
//      workerctx_t *wc = LpelWorkerGetContext(worker_id);
//      WORKER_DBGMSG(wc,
//          "Enter spmd req'd by task %u on worker %d (VId=%d).\n",
//          curreq.task->uid, cur_worker_id,
//          GetVId(worker_id, cur_worker_id)
//          );
//      curreq.func(curreq.arg);
//      WORKER_DBGMSG(wc, "Left spmd.\n");
    }
    /**********************************/

    /* clear the pointer to curreq */
    self_data->curreq = NULL;

    /*
     * "Stop-Barrier"
     */
    if (cur_worker_id == worker_id) {
      /* we are the "master" thread */
      /* wait until other threads have left the spmd */
      for (i=0; i<num_workers-1; i++) {
        sem_wait(&master_data->sema);
      }

      /* clear pending flag, now it is safe on the current worker
         to handle subsequent requests */
      assert( 1 == master_data->pending );
      master_data->pending = 0;

      /* now we can wakeup the task */
      LpelWorkerTaskWakeupLocal( curreq.task->worker_context, curreq.task);
    } else {
      /* we are NOT master, signal master before continuing */
      sem_post(&master_data->sema);
    } /* End "Stop-Barrier" */

  } /* END WHILE(1) */
}


void LpelSpmdRequest(lpel_task_t *task, lpel_spmdfunc_t fun, void *arg)
{
  int tail;
  spmdreq_t *req;
  workerctx_t *wc = task->worker_context;
  assert(wc != NULL && wc->wid >= 0 && wc->wid < num_workers);

  /* set pending flag */
  assert( 0 == worker_data[wc->wid].pending);
  worker_data[wc->wid].pending = 1;

  /* append */
  tail = fetch_and_inc(&qtail) % num_workers;
  req = &req_queue[tail];
  req->wctx = wc;
  req->func = fun;
  req->arg  = arg;
  req->task = task;
}



/**
 * Get the virtual worker id from within a spmd function
 */
int LpelSpmdVId(void)
{
  int self_id, master_id;
  spmdreq_t *curreq;

  self_id = LpelWorkerSelf()->wid;
  assert( self_id >= 0 && self_id < num_workers );

  /* if we are not in a spmd, curreq is NULL */
  curreq = worker_data[self_id].curreq;
  assert(curreq != NULL);

  /* the virtual id for the master is 0,
   * all other worker ids are shifted
   * by the master id modulo num_workers
   */
  master_id = curreq->wctx->wid;
  return GetVId(self_id, master_id);
}


