#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include "lpel.h"


typedef struct readyset readyset_t;

extern readyset_t *SchedInit(void);
extern void SchedCleanup(readyset_t *ready);
extern void SchedPutReady(readyset_t *ready, task_t *t);
extern task_t *SchedFetchNextReady(readyset_t *ready);



#endif /* _SCHEDULER_H_ */
