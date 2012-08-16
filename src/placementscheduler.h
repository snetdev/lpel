#ifndef _Placement_Scheduler_H_
#define _Placement_Scheduler_H_

#include "lpel.h"
#include "lpel/timing.h"

typedef struct task_placement task_placement_t;
typedef struct worker_placement worker_placement_t;

#ifdef WAITING
struct worker_placement {
  double avg;
};

struct task_placement {
  lpel_timing_t ready, total;
  int queued;
  double avg;
};

static inline worker_placement_t *PlacementInitWorker(void)
{
  worker_placement_t *data = malloc(sizeof(worker_placement_t));
  data->avg = 0;
  return data;
}
#define PlacementDestroyWorker(data)  free(data);

static inline task_placement_t *PlacementInitTask(void)
{
  task_placement_t *data = malloc(sizeof(task_placement_t));
  data->queued = 0;
  return data;
}
#define PlacementDestroyTask(data)    free(data)

void PlacementRunCallback(lpel_task_t *);
void PlacementReadyCallback(lpel_task_t *);
void PlacementStopCallback(lpel_task_t *);

#else

struct task_placement {
  int queued;
};

#define PlacementInitWorker(data)     NULL
#define PlacementDestroyWorker(data)

static inline task_placement_t *PlacementInitTask(void)
{
  task_placement_t *data = malloc(sizeof(task_placement_t));
  data->queued = 0;
  return data;
}
#define PlacementDestroyTask(data)    free(data)

#define PlacementRunCallback(task)
void PlacementReadyCallback(lpel_task_t *);
#define PlacementStopCallback(task)
#endif

#endif
