
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "arch/atomic.h"

#include "task.h"

#include "lpelcfg.h"
#include "worker.h"
#include "spmdext.h"
#include "stream.h"
#include "lpel/monitor.h"
#include "placementscheduler.h"
#include "taskqueue.h"
#include "lpel/timing.h"

static atomic_int taskseq = ATOMIC_VAR_INIT(0);


/* declaration of startup function */
static void TaskStartup( void *arg);
static void TaskStart( lpel_task_t *t);
static void TaskStop( lpel_task_t *t);
static void TaskDropContext( lpel_task_t *t);

#define TASK_STACK_ALIGN  256
#define TASK_MINSIZE  4096

/**
 * Create a task.
 *
 * @param worker  id of the worker where to create the task
 * @param func    task function
 * @param arg     arguments
 * @param size    size of the task, including execution stack
 * @pre           size is a power of two, >= 4096
 *
 * @return the task handle of the created task (pointer to TCB)
 *
 * TODO reuse task contexts from the worker
 */
lpel_task_t *LpelTaskCreate( int worker, lpel_taskfunc_t func,
                             void *inarg, int size)
{
  workerctx_t *wc = LpelWorkerGetContext(worker);
  lpel_task_t *t = LpelPopTask(free, &wc->free_tasks);

  if (t == NULL) {
      t = malloc(sizeof(lpel_task_t));

      t->worker_context = wc;

      /* initialize poll token to 0 */
      atomic_init( &t->poll_token, 0);
  } else {
      assert( t->stack == NULL);
      assert( t->state == TASK_ZOMBIE);
  }

  t->sched_info.prio = 0;
  t->state = TASK_CREATED;
  t->terminate = 1;
  t->prev = t->next = NULL;
  t->mon = NULL;

  t->usrdata = NULL;
  t->usrdt_destr = NULL;

  t->name = NULL;
  t->name_destr = NULL;

  t->uid = atomic_fetch_add( &taskseq, 1);  /* obtain a unique task id */
  t->func = func;
  t->inarg = inarg;
  t->current_worker = worker;
  t->new_worker = worker;

  if (size <= 0) size = LPEL_TASK_SIZE_DEFAULT;
  else if (size <= TASK_MINSIZE) size = TASK_MINSIZE;

  t->size = size;

  /* aligned to page boundary */
  t->stack = valloc( size );

  assert(t->stack != NULL);

  t->placement_data = PlacementInitTask();

  /* function, argument (data), stack base address, stacksize */
  mctx_create( &t->mctx, TaskStartup, (void*)t, t->stack, size);

#ifdef USE_MCTX_PCL
  assert(t->mctx != NULL);
#endif

  return t;
}

/**
 * Destroy a task:
 * - completely free the memory for that task
 */
void LpelTaskDestroy( lpel_task_t *t)
{
  // the task must have exited already.
  assert( t->state == TASK_ZOMBIE);

  // the context must have been dropped already.
  assert( t->stack == NULL);

#ifdef USE_TASK_EVENT_LOGGING
  /* if task had a monitoring object, destroy it */
  if (t->mon && MON_CB(task_destroy)) {
    MON_CB(task_destroy)(t->mon);
  }
#endif

  // Unwind the remaining allocation steps from TaskCreate.
  atomic_destroy( &t->poll_token);

  PlacementDestroyTask(t->placement_data);

  free(t);
}


unsigned int LpelTaskGetID(lpel_task_t *t)
{
  return t->uid;
}

mon_task_t *LpelTaskGetMon( lpel_task_t *t )
{
  return t->mon;
}


void LpelTaskMonitor(lpel_task_t *t, mon_task_t *mt)
{
  t->mon = mt;
}


void LpelTaskPrio(lpel_task_t *t, int prio)
{
  t->sched_info.prio = prio;
}

int LpelTaskGetPrio(lpel_task_t *t)
{
  return t->sched_info.prio;
}

/**
 * Let the task run on the worker
 */
void LpelTaskRun( lpel_task_t *t)
{
  assert( t->state == TASK_CREATED );

  LpelWorkerRunTask( t);
}



/**
 * Get the current task
 *
 * @pre This call must be made from within a LPEL task!
 */
lpel_task_t *LpelTaskSelf(void)
{
  return LpelWorkerCurrentTask();
}

/**
 * Exit the current task
 *
 * @param outarg  output argument of the task
 * @pre This call must be made from within a LPEL task!
 */
void LpelTaskExit(void)
{
  lpel_task_t *t = LpelTaskSelf();
  workerctx_t *wc = t->worker_context;

  if (t->state == TASK_RUNNING) {
    t->state = TASK_ZOMBIE;
    TaskStop( t);
  }

  if (t->usrdt_destr && t->usrdata) {
    t->usrdt_destr (t, t->usrdata);
  }

  if (t->name_destr && t->name) {
    t->name_destr(t->name);
  }

  // NB: can't deallocate stack here, because
  // the task is still executing; so defer deallocation
  LpelCollectTask(wc, t);

  wc->num_tasks--;
  /* wrappers can terminate if their task terminates */
  if (wc->wid < 0) {
    wc->terminate = 1;
  }

  LpelWorkerDispatcher( t);
}

/**
 * Delayed task deallocation.
 */
void LpelCollectTask(workerctx_t *wc, lpel_task_t* t)
{
    /* delete task marked before */
    if (wc->marked_del != NULL) {

        TaskDropContext(wc->marked_del); 

        LpelPushTask(free, &wc->free_tasks, wc->marked_del);

        wc->marked_del = NULL;
    }
    /* place a new task (if any) */
    if (t != NULL) {
        wc->marked_del = t;
    }    
}

static void TaskDropContext(lpel_task_t *t)
{
    assert( t->stack != NULL);
    
    free(t->stack);

    t->stack = NULL;

    // The mctx is only destroyed upon deallocation.
}

/**
 * Yield execution back to scheduler voluntarily
 *
 * @pre This call must be made from within a LPEL task!
 */
void LpelTaskYield(void)
{
  lpel_task_t *ct = LpelTaskSelf();
  assert( ct->state == TASK_RUNNING );

  ct->state = TASK_READY;
  LpelWorkerSelfTaskYield(ct);
  LpelTaskBlock( ct );
}




/**
 * Block a task
 */
void LpelTaskBlockStream(lpel_task_t *t)
{
  /* a reference to it is held in the stream */
  t->state = TASK_BLOCKED;
  LpelTaskBlock( t );
}


/**
 * Unblock a task. Called from StreamRead/StreamWrite procedures
 */
void LpelTaskUnblock( lpel_task_t *ct, lpel_task_t *blocked)
{
  assert(ct != NULL);
  assert(blocked != NULL);

  LpelWorkerTaskWakeup( ct, blocked);
}


/** 
 * Prepare for respawning the current task,
 * possibly with a different function
 * (continuation)
 */
void LpelTaskRespawn(lpel_taskfunc_t f)
{
  lpel_task_t *t = LpelTaskSelf();
  if (f) t->func = f;
  t->terminate = 0;
}


/**
 * Task issues an enter world request
 */
void LpelTaskEnterSPMD( lpel_spmdfunc_t fun, void *arg)
{
  lpel_task_t *ct = LpelTaskSelf();
  workermsg_t msg;
  assert( ct->state == TASK_RUNNING );

//FIXME conditional for availability?

  /* world request */
  LpelSpmdRequest(ct, fun, arg);

  /* broadcast message */
  msg.type = WORKER_MSG_SPMDREQ;
  msg.body.from_worker = ct->worker_context->wid;
  LpelWorkerBroadcast(&msg);

  ct->state = TASK_BLOCKED;
  /* TODO block on what? */
  LpelTaskBlock( ct );
}


/******************************************************************************/
/* PRIVATE FUNCTIONS                                                          */
/******************************************************************************/

/**
 * Startup function for user specified task,
 * calls task function with proper signature
 *
 * @param data  the previously allocated lpel_task_t TCB
 */
static void TaskStartup( void *data)
{
  lpel_task_t *t = data;

  for (;;) {
    for (;;) {
      t->state = TASK_READY;
      t->terminate = 1;

      TaskStart( t);
      /* call the task function with inarg as parameter */
      t->func(t->inarg);
      t->state = TASK_ZOMBIE;
      TaskStop( t);

      if (t->terminate) break;

      if (t->new_worker != t->current_worker) {
        lpel_task_t *new;
        new  = LpelTaskCreate(t->new_worker, t->func, t->inarg, t->size);
        new->sched_info.prio = t->sched_info.prio;
        LpelTaskRun(new);
        break;
      }
    }

    /* if task function returns, exit properly */
    LpelTaskExit();
  }
}

static void TaskStart( lpel_task_t *t)
{
  assert( t->state == TASK_READY );

  /* MONITORING CALLBACK */
#ifdef USE_TASK_EVENT_LOGGING
  if (t->mon && MON_CB(task_start)) {
    MON_CB(task_start)(t->mon);
  }
#endif

  t->state = TASK_RUNNING;
  PlacementRunCallback(t);
}


static void TaskStop( lpel_task_t *t)
{
  assert( t->state != TASK_RUNNING);

  /* MONITORING CALLBACK */
#ifdef USE_TASK_EVENT_LOGGING
  if (t->mon && MON_CB(task_stop)) {
    MON_CB(task_stop)(t->mon, t->state);
  }
#endif

}

void LpelTaskBlock( lpel_task_t *t )
{
  assert( t->state != TASK_RUNNING);

  TaskStop( t);
  PlacementStopCallback(t);
  LpelWorkerDispatcher( t);
  TaskStart( t);
}

/** user data */
void  LpelSetUserData(lpel_task_t *t, void *data)
{
  assert(t);
  t->usrdata = data;
}

void *LpelGetUserData(lpel_task_t *t)
{
  assert(t);
  return t->usrdata;
}

void LpelSetUserDataDestructor(lpel_task_t *t, lpel_usrdata_destructor_t destr)
{
  assert(t);
  t->usrdt_destr = destr;
}

lpel_usrdata_destructor_t LpelGetUserDataDestructor(lpel_task_t *t)
{
  assert(t);
  return t->usrdt_destr;
}

void LpelSetName(lpel_task_t *t, const char *name) { t->name = name; }

const char *LpelGetName(lpel_task_t *t) { return t->name; }

void LpelSetNameDestructor(lpel_task_t *t, void (*f)(void *))
{ t->name_destr = f; }


int LpelTaskGetWorkerId(lpel_task_t *t)
{
  assert(t);
  return t->worker_context->wid;
}
