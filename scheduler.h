#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include "lpel.h"

struct task;

typedef struct schedcfg schedcfg_t;

typedef struct schedctx schedctx_t;

extern void SchedInit(int size, schedcfg_t *cfg);
extern void SchedCleanup(void);

extern void SchedAssignTask( struct task *t);
//extern void SchedWrapper( lpelthread_t *env, void *arg);
extern void SchedWakeup( struct task *by, struct task *whom);


#endif /* _SCHEDULER_H_ */
