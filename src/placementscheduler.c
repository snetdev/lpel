#include <stdlib.h>
#include <stdio.h>

#include <pthread.h>

#include "placementscheduler.h"
#include "scheduler.h"
#include "worker.h"
#include "taskiterator.h"

#ifdef WAITING
#define MAX_PERCENTAGE 0.5
#define LOW_LOAD 0.3
#endif


typedef struct lpel_worker_indices {
  int * workers;
  int n;
} lpel_worker_indices_t;

lpel_worker_indices_t task_types[SCHED_NUM_PRIO];

#ifdef WAITING
double *worker_percentages;
#endif

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

static void CalculateAverageReadyTime(workerctx_t **workers,
                                      int num_workers,
                                      pthread_mutex_t *mutex_workers)
{
  int i;
  for(i = 0; i < num_workers; i++) {
    double worker_percentage = 0;
    int n_tasks;
    pthread_mutex_lock(&mutex_workers[i]);
    lpel_task_iterator_t *iter = LpelSchedTaskIter(workers[i]->sched);
    pthread_mutex_unlock(&mutex_workers[i]);

    while(LpelTaskIterHasNext(iter)) {
      lpel_task_t *t;
      double ready_percentage;

      t = LpelTaskIterNext(iter);

      ready_percentage = LpelTaskGetPercentageReady(t);
      if(ready_percentage > MAX_PERCENTAGE) {
        worker_percentages[i] = -1;
        break;
      } else {
        worker_percentages[i] += ready_percentage;
        n_tasks++;
      }
    }
    if(worker_percentages[i] >= 0) {
      worker_percentages[i] = worker_percentages[i] / (double)n_tasks;
    }
    LpelTaskIterDestroy(iter);
  }
}

static void FindWorker(lpel_task_t *t)
{
  int i;
  int prio;
#ifdef TASK_WORKERS_SEPARATION
  prio = LpelTaskGetPrio(t);
#else
  prio = 0;
#endif
  for(i = 0; i<task_types[prio].n; i++) {
    int wid = task_types[prio].workers[i];
    if(worker_percentages[wid] >= 0 && worker_percentages[i] < LOW_LOAD) {
      t->new_worker = wid;
      return;
    }
  }
  t->new_worker = t->current_worker;
}

static void WaitingPlacement(workerctx_t **workers,
                             int num_workers,
                             pthread_mutex_t *mutex_workers)
{
  int i;
  CalculateAverageReadyTime(workers, num_workers, mutex_workers);

  //TODO ADD TASK SEGMENTATION
  for(i = 0; i<num_workers; i++) {
    pthread_mutex_lock(&mutex_workers[i]);
    lpel_task_iterator_t *iter = LpelSchedTaskIter(workers[i]->sched);
    pthread_mutex_unlock(&mutex_workers[i]);
    while(LpelTaskIterHasNext(iter)) {
      lpel_task_t *t;
      double ready_percentage;

      t = LpelTaskIterNext(iter);

      ready_percentage = LpelTaskGetPercentageReady(t);

      if(ready_percentage > MAX_PERCENTAGE) {
        FindWorker(t);
      }
    }
    LpelTaskIterDestroy(iter);
  }
}
#endif

void LpelPlacementSchedulerDestroy()
{
  int i;
  for(i = 0; i<SCHED_NUM_PRIO; i++) {
    if(task_types[i].n > 0) {
      free(task_types[i].workers);
    }
  }
  free(worker_percentages);
}

void LpelPlacementSchedulerInit()
{
  int i;
  int number_workers = LpelWorkerNumber();
#ifdef WAITING
  worker_percentages = malloc(number_workers * sizeof(double));
  for(i = 0; i<number_workers; i++) {
    worker_percentages[i] = -1;
  }
#endif

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
#ifdef WAITING
    WaitingPlacement(workers, LpelWorkerNumber(), mutex_workers);
#else
    int i;

    for(i = 0; i<LpelWorkerNumber(); i++) {
      pthread_mutex_trylock(&mutex_workers[i]);
      if(workers[i]->terminate == 0) {
        RandomPlacement(workers[i]);
      }
      pthread_mutex_unlock(&mutex_workers[i]);
    }
#endif

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
