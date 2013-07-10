#ifndef _DECEN_SCHEDULER_H_
#define _DECEN_SCHEDULER_H_

#include "lpel.h"

#define SCHED_NUM_PRIO  2

typedef struct schedctx_t schedctx_t;

typedef struct {
  int prio;
} sched_task_t;


schedctx_t *LpelSchedCreate( int wid);
void LpelSchedDestroy( schedctx_t *sc);

void LpelSchedMakeReady( schedctx_t* sc, lpel_task_t *t);
struct lpel_task_t *LpelSchedFetchReady( schedctx_t *sc);



#endif /* _DECEN_SCHEDULER_H_ */
