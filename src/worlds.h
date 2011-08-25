#ifndef _WORLDS_H_
#define _WORLDS_H_

#include <lpel.h>
#include "task.h"



int  LpelWorldsInit(int num_workers);
void LpelWorldsCleanup(void);

void LpelWorldsHandleRequests(int worker_id);
void LpelWorldsRequest(lpel_task_t *task, lpel_worldfunc_t, void *arg);




#endif /* _WORLDS_H_ */
