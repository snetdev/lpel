#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_


struct lpel_task_t;

#define SCHED_NUM_PRIO  2

typedef struct schedctx schedctx_t;

typedef struct {
  int prio;
} sched_task_t;


schedctx_t *SchedCreate( int wid);
void SchedDestroy( schedctx_t *sc);

void SchedMakeReady( schedctx_t* sc, struct lpel_task_t *t);
struct lpel_task_t *SchedFetchReady( schedctx_t *sc);



#endif /* _SCHEDULER_H_ */
