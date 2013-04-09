#ifndef _SPMDEXT_H_
#define _SPMDEXT_H_

#include <decen_lpel.h>
#include "task.h"



int  LpelSpmdInit(int num_workers);
void LpelSpmdCleanup(void);

void LpelSpmdHandleRequests(int worker_id);
void LpelSpmdRequest(lpel_task_t *task, lpel_spmdfunc_t, void *arg);




#endif /* _SPMDEXT_H_ */
