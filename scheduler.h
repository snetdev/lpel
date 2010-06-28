#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include "lpel.h"

extern void *SchedInit(void);
extern void SchedPutReady(void *ready, task_t *t);
extern task_t *SchedFetchNextReady(void *ready);



#endif /* _SCHEDULER_H_ */
