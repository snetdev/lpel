#include <stdlib.h>
#include <stdio.h>

#include <pthread.h>

#include "placementscheduler.h"
#include "scheduler.h"
#include "worker.h"
#include "taskiterator.h"


typedef struct lpel_worker_indices {
  int * workers;
  int n;
} lpel_worker_indices_t;

lpel_worker_indices_t task_types[SCHED_NUM_PRIO];

static void RandomPlacement(workerctx_t *wc)
{
  lpel_task_iterator_t *iter = LpelSchedTaskIter(wc->sched);
  while(LpelTaskIterHasNext(iter)) {
    lpel_task_t *t;
    int current_worker;
    double c;
    int migrate;
#ifdef TASK_WORKER_SEPARATION
    int prio;
#endif

    t = LpelTaskIterNext(iter);
    current_worker = t->current_worker;

    c = (double)rand() / (double)RAND_MAX;
    migrate = (c < 0.5) ? 1 : 0;
#ifdef TASK_WORKER_SEPARATION
    assert(prio < 2);
    prio = LpelTaskGetPrio(t);
    t->new_worker = c ? task_types[prio].workers[(rand() %
                        task_types[prio].n)] : current_worker;
#else
    t->new_worker = c ? task_types[0].workers[(rand() %
                        task_types[0].n)] : current_worker;
#endif
  }
  LpelTaskIterDestroy(iter);
}

#ifdef WAITING
typedef struct lpel_worker_stats {
  lpel_worker_stats_t *next;
  workerctx_t *wc;
  double average_percentage_ready;
} lpel_worker_stats_t;

typedef struct lpel_migration_task {
  lpel_migration_task_t *next;
  lpel_task_t *t;
  double percentage_ready;
} lpel_migration_task_t;

//static void WaitingPlacement(workerctx_t *wc
#endif

void LpelPlacementSchedulerDestroy()
{
  int i;
  for(i = 0; i<SCHED_NUM_PRIO; i++) {
    if(task_types[i].n > 0) {
      free(task_types[i].workers);
    }
  }
}

void LpelPlacementSchedulerInit()
{
  int i;
  int number_workers = LpelWorkerNumber();

#ifdef TASK_WORKER_SEPARATION
  assert(SCHED_NUM_PRIO >= 2);
  assert(number_workers >= 2);
  int num1_workers = (number_workers/6 > 0) ? number_workers/6 : 1;
  int num0_workers = number_workers-num1_workers;
  int num0_i;
  int num1_i;

  task_types[0].workers = malloc(num0_workers * sizeof(int));
  task_types[0].n = num0_workers;
  task_types[1].workers = malloc(num1_workers * sizeof(int));
  task_types[1].n = num1_workers;

  for(i = 0, num0_i = 0, num1_i = 0; i < number_workers; i++) {
    if(i % number_workers == 0) {
      task_types[1].workers[num1_i] = i;
      num1_i++;
    } else {
      task_types[0].workers[num0_i] = i;
      num0_i++;
    }
  }

  for(i = 2; i<SCHED_NUM_PRIO; i++) {
    task_types[i].n = 0;
  }

#else
  assert(SCHED_NUM_PRIO >= 1);
  task_types[0].workers = malloc(number_workers * sizeof(int));
  task_types[0].n = number_workers;
  for(i = 0; i < task_types[0].n; i++) {
    task_types[0].workers[i] = i;
  }

  for(i = 1; i<SCHED_NUM_PRIO; i++) {
    task_types[i].n = 0;
  }
#endif

  if(number_workers >= 2) {
    lpel_task_t *t = LpelTaskCreate(number_workers,
                                    0,
                                    &LpelPlacementSchedulerRun,
                                    NULL,
                                    256 * 1024);
    LpelTaskRun(t);
  }

}

void * LpelPlacementSchedulerRun(void * args)
{
  workerctx_t *wc = LpelTaskSelf()->worker_context;
  do {
    workerctx_t **workers = LpelWorkerGetWorkers();
    pthread_mutex_t *mutex_workers = LpelWorkerGetMutexes();
    int i;

    for(i = 0; i<LpelWorkerNumber(); i++) {
      pthread_mutex_trylock(&mutex_workers[i]);
      if(workers[i]->terminate == 0) {
        RandomPlacement(workers[i]);
      }
      pthread_mutex_unlock(&mutex_workers[i]);
    }


    LpelTaskYield();
  } while(!wc->terminate);
}

int LpelPlacementSchedulerGetWorker(int prio, int i)
{
#ifdef TASK_WORKER_SEPARATION
  assert(prio < 2);
  return task_types[prio].workers[i];
#else
  return task_types[0].workers[i];
#endif
}

int LpelPlacementSchedulerNumWorkers(int prio)
{
#ifdef TASK_WORKER_SEPARATION
  assert(prio < 2);
  return task_types[prio].n;
#else
  return task_types[0].n;
#endif
}

int LpelPlacementSchedulerGetIndexWorker(int prio, int worker)
{
  int i;
#ifdef TASK_WORKER_SEPARATION
  assert(prio < 2);
  for(i = 0; i<task_types[prio].n; i++) {
    if(task_types[prio].workers[i] == worker) {
      return i;
    }
  }
  return -1;
#else
  for(i = 0; i<task_types[0].n; i++) {
    if(task_types[prio].workers[i] == worker) {
      return i;
    }
  }
  return -1;
#endif
}
