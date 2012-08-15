#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <pthread.h>

#include "lpelcfg.h"
#include "placementscheduler.h"
#include "scheduler.h"
#include "worker.h"
#include "taskiterator.h"


#ifdef WAITING
#define MAX_PERCENTAGE 1
#ifdef TASK_SEGMENTATION
#define LISTS_LENGTH 2
#else
#define LISTS_LENGTH 1
#endif
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
} lpel_task_list_t;

typedef struct lpel_steal_workers{
  int *workers;
  int length;
} lpel_steal_workers_t;

double *worker_percentages;
lpel_task_list_t task_list;
lpel_steal_workers_t steal_workers[LISTS_LENGTH];
#endif

static lpel_worker_indices_t task_types[SCHED_NUM_PRIO];
static float threshold;


#ifdef WAITING
static void AddStealingWorkerId(int i, int prio)
{
  steal_workers[prio].workers[steal_workers[prio].length] = i;
  steal_workers[prio].length++;
}

static int GetWorkerId(int prio)
{
  int wid;
  int r = rand()%steal_workers[prio].length;
  wid = steal_workers[prio].workers[r];
  if(steal_workers[prio].length == 0) {
    return -1;
  }
  steal_workers[prio].length--;
  steal_workers[prio].workers[r] =
          steal_workers[prio].workers[steal_workers[prio].length];
  return wid;
}

static void ResetWorkers()
{
  int i;
  for(i = 0; i < LISTS_LENGTH; i++) {
    steal_workers[i].length = 0;
  }
}

static void AddTaskToList(lpel_task_t *t, double stat)
{
  int i;
  //first find place to add the stat
  //FIXME add more efficient algorithm to find the index
  if(task_list.length == 0) {
    task_list.list[0].t = t;
    task_list.list[0].stat = stat;
    task_list.length++;
  }else {
    int is_added = 0;
    for(i = 0; i<task_list.length; i++) {
      if(task_list.list[i].stat <= stat) {
        is_added = 1;
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
        if(temp_t != NULL) {
          temp_t->new_worker = temp_t->current_worker;
        }
        break;
      }
    }

    /* all other tasks in list have higher ready time and task is not added
     * set new_worker to current_worker
     */
    if(!is_added) {
      t->new_worker = t->current_worker;
    }
  }
}

/**
 * Returns the first task in the list.
 *
 */
static lpel_task_t * GetTask(int index)
{
  lpel_task_t *t = task_list.list[index].t;
  task_list.list[index].t = NULL;
  return t;
}

/**
 * Create a list of tasks that are to be used for migration.
 *
 * @param workers       The workers array
 * @param num_workers   The length of the array
 * @param mutex_workers The mutexes for the workers
 */
static void CreateTaskList(workerctx_t **workers,
                           int num_workers,
                           pthread_mutex_t *mutex_workers)
{
  int i;
  for(i = 0; i<num_workers; i++) {
    lpel_task_iterator_t *iter;

    /* workers with a negative percentage are asking for work and don't
     * hand tasks over to other workers
     */
    if(worker_percentages[i] <= 0) {
      continue;
    }

    /* start getting suitable tasks from workers */
    pthread_mutex_lock(&mutex_workers[i]);
    LpelSchedLockQueue(workers[i]->sched);
    iter = LpelSchedTaskIter(workers[i]->sched);

    while(LpelTaskIterHasNext(iter)) {
      lpel_task_t *t;
      double ready_ratio;

      t = LpelTaskIterNext(iter);

      if(t->state != TASK_READY) {
        continue;
      }

      ready_ratio = LpelTaskGetPercentageReady(t);
      /* add task to queue for migration to another worker */
      if(ready_ratio >= worker_percentages[i] * threshold) {
        AddTaskToList(t, ready_ratio);
      } else {
        t->new_worker = t->current_worker;
      }
    }
    LpelSchedUnlockQueue(workers[i]->sched);
    pthread_mutex_unlock(&mutex_workers[i]);

  }
}

/**
 * Remove any tasks that are not used for migration, set the new_worker of
 * the task to the current worker. Reset the list
 */
static void ResetTaskList()
{
  int i;
  for(i = 0; i<task_list.length; i++) {
    task_list.list[i].t = NULL;
  }
  task_list.length = 0;
}

/**
 * For each worker go through all tasks that are in the ready list and
 * sum up all waiting times from all tasks and average them. If a worker
 * is tagged waiting (ie, there weren't any tasks in the ready queue to run)
 * it will not calculate the average but instead add its id to the list
 * of workers that need tasks. The average is set to -1.
 * Note to make, average is multiplied by some value. This is the threshold
 * value for tasks that are chosen for migration.
 *
 * @param workers         The workers array
 * @param num_workers     The number of workers
 * @param mutex_workers   The mutexes for the workers
 */
static void CalculateAverageReadyTime(workerctx_t **workers,
                                      int num_workers,
                                      pthread_mutex_t *mutex_workers)
{
  int i;
  int prio = 0;
  for(i = 0; i < num_workers; i++) {
    double worker_percentage = 0;
    int n_tasks = 0;
    lpel_task_iterator_t *iter;

    pthread_mutex_lock(&mutex_workers[i]);
#ifdef TASK_SEGMENTATION
    prio = workers[i]->task_type;
#endif
    if(workers[i]->terminate) {
      pthread_mutex_unlock(&mutex_workers[i]);
      return;
    }
    if(workers[i]->waiting) {
      workers[i]->waiting = 0;
      worker_percentages[i] = -1;
      AddStealingWorkerId(i, prio);
      pthread_mutex_unlock(&mutex_workers[i]);
      continue;
    }
    LpelSchedLockQueue(workers[i]->sched);
    iter = LpelSchedTaskIter(workers[i]->sched);


    while(LpelTaskIterHasNext(iter)) {
      lpel_task_t *t;
      double ready_percentage;

      t = LpelTaskIterNext(iter);
      if(t->state != TASK_READY) {
        continue;
      }

      ready_percentage = LpelTaskGetPercentageReady(t);
      worker_percentage += ready_percentage;
      n_tasks++;
    }
    if(n_tasks > 0) {
      worker_percentages[i] = worker_percentage / (double)n_tasks;
    } else {
      worker_percentages[i] = -1;
      AddStealingWorkerId(i, prio);
    }
    LpelTaskIterDestroy(iter);

    LpelSchedUnlockQueue(workers[i]->sched);
    pthread_mutex_unlock(&mutex_workers[i]);
  }
}

static void WaitingPlacement(workerctx_t **workers,
                             int num_workers,
                             pthread_mutex_t *mutex_workers)
{
  int j;
  int i;
  int stealing_workers_num;
  /* Calculate averages of ready time for all workers */
  CalculateAverageReadyTime(workers, num_workers, mutex_workers);
  CreateTaskList(workers, num_workers, mutex_workers);

  for(i = 0; i < LISTS_LENGTH; i++) {
    stealing_workers_num = steal_workers[i].length;

    j = 0;

    while(steal_workers[i].length > 0) {
      int wid = GetWorkerId(i);
      int counter;
      for(counter = j; counter < task_list.length;
          counter = counter + stealing_workers_num) {
        task_list.list[j].t->new_worker = wid;
      }
      j++;
    }
  }

  ResetWorkers();
  ResetTaskList();
}
#else
static void RandomPlacement(workerctx_t *wc)
{
  LpelSchedLockQueue(wc->sched);
  lpel_task_iterator_t *iter = LpelSchedTaskIter(wc->sched);
  while(LpelTaskIterHasNext(iter)) {
    lpel_task_t *t;
    int current_worker;
    double c;
    int migrate;
#ifdef TASK_SEGMENTATION
    int prio = wc->task_type;
#endif

    t = LpelTaskIterNext(iter);
    current_worker = t->current_worker;

    c = (double)rand() / (double)RAND_MAX;
    migrate = (c > threshold) ? 1 : 0;
#ifdef TASK_SEGMENTATION
    assert(prio < 2);
    prio = LpelTaskGetPrio(t);
    t->new_worker = migrate ? task_types[prio].workers[(rand() %
                              task_types[prio].n)] : t->current_worker;
#else
    t->new_worker = migrate ? task_types[0].workers[(rand() %
                              task_types[0].n)] : t->current_worker;
#endif
  }
  LpelTaskIterDestroy(iter);
  LpelSchedUnlockQueue(wc->sched);
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
  for( i = 0; i < LISTS_LENGTH; i++) {
    free(steal_workers[i].workers);
  }
  free(task_list.list);
#endif
}

void LpelPlacementSchedulerInit()
{
  lpel_config_t *config = &_lpel_global_config;
  int i;
  lpel_task_t *t;
  int number_workers = LpelWorkerNumber();
  threshold = (config->threshold >= 0) ? config->threshold : 1;

#ifdef WAITING
  worker_percentages = malloc(number_workers * sizeof(double));
  for(i = 0; i < number_workers; i++) {
    worker_percentages[i] = -1;
  }

  for(i = 0; i < LISTS_LENGTH; i++) {
    steal_workers[i].workers = malloc(number_workers * sizeof(int));
    steal_workers[i].length = 0;
  }

  task_list.list = malloc(number_workers * 2 * sizeof(lpel_task_stat_t));
  task_list.real_length = number_workers * 2;
  task_list.length = 0;
  for(i = 0; i<task_list.real_length; i++) {
    task_list.list[i].t = NULL;
  }

#endif

#ifdef TASK_SEGMENTATION
  assert(SCHED_NUM_PRIO >= 2);
  assert(number_workers >= 2);
  int num1_workers = (config->segmentation > 0) ? config->segmentation : 1;
  int num0_workers = number_workers-num1_workers;
  int num0_i;
  int num1_i;

  task_types[0].workers = malloc(num0_workers * sizeof(int));
  task_types[0].n = num0_workers;
  task_types[1].workers = malloc(num1_workers * sizeof(int));
  task_types[1].n = num1_workers;

  for(i = 0; i < number_workers; i++) {
    if(i < num0_workers) {
      task_types[0].workers[i] = i;
      LpelWorkerSetTaskType(i, 0);
    } else {
      int index = i - num0_workers;
      task_types[1].workers[index] = i;
      LpelWorkerSetTaskType(i, 1);
    }
    /*
    if(i % number_workers == 0) {
      task_types[1].workers[num1_i] = i;
      LpelWorkerSetTaskType(i, 1);
      num1_i++;
    } else {
      task_types[0].workers[num0_i] = i;
      LpelWorkerSetTaskType(i, 0);
      num0_i++;
    }*/
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
#ifdef SCHEDULER_CONCURRENT_PLACEMENT
  if(number_workers >= 2) {
    lpel_task_t *t = LpelTaskCreate(number_workers,
                                    &LpelPlacementSchedulerRun,
                                    NULL,
                                    256 * 1024);
    LpelTaskRun(t);
  }
#else
  t = LpelTaskCreate(0,
                     &LpelPlacementSchedulerRun,
                     NULL,
                     256 * 1024);
#endif
    LpelTaskRun(t);

}

void LpelPlacementSchedulerRun(void * args)
{
  workerctx_t *wc = LpelTaskSelf()->worker_context;
  workerctx_t **workers = LpelWorkerGetWorkers();
  pthread_mutex_t *mutex_workers = LpelWorkerGetMutexes();
  int num_workers = LpelWorkerNumber();
  int terminate;
  do {
    int tasks;

    if(wc->terminate) {
      break;
    }
#ifdef WAITING
   WaitingPlacement(workers, num_workers, mutex_workers);
#else
    int i;

    for(i = 0; i<num_workers; i++) {
      int status = pthread_mutex_trylock(&mutex_workers[i]);
      if(status == 0) {
        if(!workers[i]->terminate) {
          RandomPlacement(workers[i]);
        } else {
          pthread_mutex_unlock(&mutex_workers[i]);
          pthread_mutex_unlock(&mutex_workers[wc->wid]);
          return;
        }
        pthread_mutex_unlock(&mutex_workers[i]);
      }
    }
#endif
    usleep(100);
    LpelTaskYield();
    terminate = wc->terminate;
  } while(!terminate);
}

int LpelPlacementSchedulerGetWorker(int prio, int i)
{
#ifdef TASK_SEGMENTATION
  assert(prio < 2);
  return task_types[prio].workers[i];
#else
  return task_types[0].workers[i];
#endif
}

int LpelPlacementSchedulerNumWorkers(int prio)
{
#ifdef TASK_SEGMENTATION
  assert(prio < 2);
  return task_types[prio].n;
#else
  return task_types[0].n;
#endif
}

int LpelPlacementSchedulerGetIndexWorker(int prio, int worker)
{
  int i;
#ifdef TASK_SEGMENTATION
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
