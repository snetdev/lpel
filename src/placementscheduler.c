#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <pthread.h>

#include "lpel/timing.h"
#include "lpelcfg.h"
#include "placementscheduler.h"
#include "taskqueue.h"
#include "worker.h"

static double threshold = 0;
static atomic_voidptr migrate_queue = ATOMIC_VAR_INIT(NULL);

#ifdef WAITING
static double global_avg = 1;
static int *worker_indexes = NULL;

void PlacementReadyCallback(lpel_task_t *t)
{
  LpelTimingStart(&t->placement_data->ready);
  LpelTimingStart(&t->placement_data->total);
}

void PlacementRunCallback(lpel_task_t *t)
{ LpelTimingEnd(&t->placement_data->ready); }

void PlacementStopCallback(lpel_task_t *t)
{
  double wait_time, a = 0.1, b = 0.1; //FIXME
  task_placement_t *task = t->placement_data;
  worker_placement_t *worker = t->worker_context->placement_data;

  LpelTimingEnd(&task->total);

  // Placing wrappers is non-sensical
  if (t->worker_context->wid == -1) return;

  wait_time = LpelTimingToNSec(&task->ready) / LpelTimingToNSec(&task->total);
  task->avg = a * wait_time + (1-a) * task->avg;

  worker->avg = b * wait_time + (1-b) * worker->avg;

  if (task->avg > worker->avg && worker->avg > global_avg && !task->queued) {
    task->queued = 1;
    LpelPushTask(migrate, &migrate_queue, t);
  }
}

static inline int comp(const void *a, const void *b) {
  double i = workers[*(int*)a]->placement_data->avg;
  double j = workers[*(int*)b]->placement_data->avg;

  if (i < j) return -1;
  if (i > j) return 1;
  return 0;
}
#else
void PlacementReadyCallback(lpel_task_t *t)
{
  double c = (double) rand() / (double) RAND_MAX;
  if (c > threshold && !t->placement_data->queued) {
    t->placement_data->queued = 1;
    LpelPushTask(migrate, &migrate_queue, t);
  }
}
#endif

void LpelPlacementScheduler(void *arg)
{
  lpel_config_t *config = arg;
  workerctx_t *wc = LpelTaskSelf()->worker_context;

#ifdef WAITING
  double new_avg = 0;

  for (int i = 0; i < config->num_workers; i++) {
    new_avg += workers[i]->placement_data->avg;
  }

  global_avg = new_avg / config->num_workers;

  qsort(worker_indexes, config->num_workers, sizeof(int), &comp);
  //FIXME calculate weighted round robin
#endif

  lpel_task_t *t;
  while ((t = LpelPopTask(migrate, &migrate_queue))) {
#ifdef WAITING
    //FIXME select based on round robin above
    t->new_worker = worker_indexes[0];
#else
    t->new_worker = rand() % config->num_workers;
#endif
  }

  LpelTaskYield();

  if (wc->terminate) {
#ifdef WAITING
     free(worker_indexes);
#endif
     return;
  }

  LpelTaskRespawn(NULL);
}

void LpelPlacementSchedulerInit()
{
  lpel_config_t *config = &_lpel_global_config;
  threshold = config->threshold > 0 ? config->threshold : 1;

#ifdef WAITING
  worker_indexes = malloc(config->num_workers * sizeof(int));
  for (int i = 0; i < config->num_workers; i++) {
    worker_indexes[i] = i;
  }
#endif

  lpel_task_t *t = LpelTaskCreate(
#ifdef SCHEDULER_CONCURRENT_PLACEMENT
      -1,
#else
      0,
#endif
      &LpelPlacementScheduler,
      config,
      256 * 1024);

  LpelTaskRun(t);
}
