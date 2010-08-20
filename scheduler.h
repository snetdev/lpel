#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include "task.h"

typedef struct schedctx schedctx_t;

typedef struct schedcfg schedcfg_t;


extern void SchedInit(int size, schedcfg_t *cfg);
extern schedctx_t *SchedGetContext(int id);
extern void SchedCleanup(void);


extern void SchedPutReady(schedctx_t *sc, task_t *t);
extern task_t *SchedFetchNextReady(schedctx_t *sc);
extern void SchedReschedule(schedctx_t *sc, task_t *t);

extern schedctx_t *SchedAddTaskGlobal(task_t *t);

#endif /* _SCHEDULER_H_ */
