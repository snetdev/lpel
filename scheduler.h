#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include "task.h"
#include "monitoring.h"

typedef struct schedctx schedctx_t;

typedef struct schedcfg schedcfg_t;


extern schedctx_t *SchedCtxCreate(schedcfg_t *cfg);
extern void SchedCtxDestroy(schedctx_t *sc);

extern void SchedAssignTask(schedctx_t *sc, task_t *t);
extern void SchedTask(schedctx_t *sc, monitoring_t *mon);


#endif /* _SCHEDULER_H_ */
