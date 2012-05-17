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


#ifdef WAITING
typedef struct lpel_task_stat{
  lpel_task_t *t;
  double stat;
} lpel_task_stat_t;

typedef struct lpel_task_list{
  lpel_task_stat_t *list;
  int length;
  int real_length;
  int head;
} lpel_task_list_t;

typedef struct lpel_steal_workers{
  int *workers;
  int length;
} lpel_steal_workers_t;

double *worker_percentages;
lpel_task_list_t task_list;
lpel_steal_workers_t steal_workers;
#endif

lpel_worker_indices_t task_types[SCHED_NUM_PRIO];

static void RandomPlacement(workerctx_t *wc)
{
  lpel_task_iterator_t *iter = LpelSchedTaskIter(wc->sched, 1);
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
static void AddStealingWorkerId(int i)
{
  steal_workers.workers[steal_workers.length] = i;
  steal_workers.length++;
}

static int GetWorkerId()
{
  int wid;
  int r = rand()%steal_workers.length;
  wid = steal_workers.workers[r];
  if(steal_workers.length == 0) {
    return -1;
  }
  steal_workers.length--;
  steal_workers.workers[r] = steal_workers.workers[steal_workers.length];
  return wid;
}

static void AddTaskToList(lpel_task_t *t, double stat)
{
  int i;
  //first find place to add the stat
  //FIXME add more efficient algorithm to find the index
  if(task_list.length == 0) {
    task_list.list[0].t = t;
    task_list.list[0].stat = stat;
  }else {
    for(i = 0; i<task_list.length; i++) {
      if(task_list.list[i].stat <= stat) {
        lpel_task_t *temp_t = task_list.list[i].t;
        double temp_stat = task_list.list[i].stat;

        task_list.list[i].t = t;
        task_list.list[i].stat = stat;

        if(task_list.real_length > task_list.length) {
          task_list.length++;
        }
        //Then put every task after this task one place to the right
        for(i = i+1; i<task_list.length && i<task_list.real_length; i++) {
          lpel_task_t *newtemp_t = task_list.list[i].t;
          double newtemp_stat = task_list.list[i].stat;
          task_list.list[i].t = temp_t;
          task_list.list[i].stat = temp_stat;
          temp_t = newtemp_t;
          temp_stat = newtemp_stat;
        }
        if(t != NULL) {
          temp_t->new_worker = temp_t->current_worker;
        }
      }
    }
  }
}

static lpel_task_t * GetTask()
{
  if(task_list.head == task_list.length) {
    return NULL;
  } else {
    lpel_task_t *t = task_list.list[task_list.head].t;
    task_list.list[task_list.head].t = NULL;
    task_list.head++;
    return t;
  }
}

static void ResetTaskList()
{
  int i;
  for(i = task_list.head; i<task_list.length; i++) {
    task_list.list[i].t->new_worker = task_list.list[i].t->current_worker;
    task_list.list[i].t = NULL;
  }
  task_list.length = 0;
  task_list.head = 0;
}

static void CalculateAverageReadyTime(workerctx_t **workers,
                                      int num_workers,
                                      pthread_mutex_t *mutex_workers)
{
  int i;
  for(i = 0; i < num_workers; i++) {
    double worker_percentage = 0;
    int n_tasks;

    pthread_mutex_lock(&mutex_workers[i]);
    if(workers[i]->waiting) {
      workers[i]->waiting = 0;
      worker_percentages[i] = -1;
      continue;
    }
    lpel_task_iterator_t *iter = LpelSchedTaskIter(workers[i]->sched, 1);

    pthread_mutex_unlock(&mutex_workers[i]);

    while(LpelTaskIterHasNext(iter)) {
      lpel_task_t *t;
      double ready_percentage;

      t = LpelTaskIterNext(iter);

      ready_percentage = LpelTaskGetPercentageReady(t);
      worker_percentages[i] += ready_percentage;
      n_tasks++;
    }
    worker_percentages[i] = worker_percentages[i] / (double)n_tasks;
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

  for(i = 0; i<num_workers; i++) {
    pthread_mutex_lock(&mutex_workers[i]);
    lpel_task_iterator_t *iter = LpelSchedTaskIter(workers[i]->sched, 0);
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
#ifdef WAITING
  free(worker_percentages);
  free(task_list.list);
#endif
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

  steal_workers.workers = malloc(number_workers * sizeof(int));
  steal_workers.length = 0;

  task_list.list = malloc(number_workers * 2 * sizeof(lpel_task_stat_t));
  task_list.real_length = number_workers * 2;
  task_list.length = 0;
  task_list.head = 0;
  for(i = 0; i<task_list.real_length; i++) {
    task_list.list[i].t = NULL;
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
