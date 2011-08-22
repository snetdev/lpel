#ifndef _WORLDS_H_
#define _WORLDS_H_

#include <lpel.h>
#include "task.h"



int  LpelWorldsInit(int num_workers);
void LpelWorldsCleanup(void);

void LpelWorldsHandleRequest(int worker_id);
void LpelWorldsRequest(int worker_id, lpel_worldfunc_t, void *arg,
    lpel_task_t *task);




#endif /* _WORLDS_H_ */
