#ifndef _WORKER_H_
#define _WORKER_H_

#include <pthread.h>
#include <lpel_common.h>

#include "arch/mctx.h"
#include "task.h"
#include "mailbox.h"


typedef struct workerctx_t workerctx_t;




#ifdef LPEL_DEBUG_WORKER
/* use the debug callback if available to print a debug message */
#define WORKER_DBGMSG(wc,...) do {\
  if ((wc)->mon && MON_CB(worker_debug)) { \
    MON_CB(worker_debug)( (wc)->mon, ##__VA_ARGS__ ); \
  }} while(0)

#else /* LPEL_DEBUG_WORKER */
#define WORKER_DBGMSG(wc,...) /*NOP*/
#endif /* LPEL_DEBUG_WORKER */





void LpelWorkersInit( int size);
void LpelWorkersCleanup( void);
void LpelWorkersSpawn(void);
void LpelWorkersTerminate(void);

void LpelWorkerRunTask( lpel_task_t *t);
void LpelWorkerDispatcher( lpel_task_t *t);

void LpelWorkerBroadcast(workermsg_t *msg);
workerctx_t *LpelWorkerGetContext(int id);
workerctx_t *LpelWorkerSelf(void);
lpel_task_t *LpelWorkerCurrentTask(void);
int LpelWorkerIsWrapper(workerctx_t *wc);

void LpelWorkerSelfTaskExit(lpel_task_t *t);
void LpelWorkerSelfTaskYield(lpel_task_t *t);
void LpelWorkerTaskBlock(lpel_task_t *t);


#endif /* _WORKER_H_ */
