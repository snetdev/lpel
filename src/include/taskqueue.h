#ifndef _TASKQUEUE_H_
#define _TASKQUEUE_H_

#include "task.h"


typedef struct taskqueue_t taskqueue_t;

taskqueue_t *LpelTaskqueueInit();

void LpelTaskqueuePush(  taskqueue_t *tq, lpel_task_t *t);

lpel_task_t *LpelTaskqueuePop( taskqueue_t *tq);


int LpelTaskqueueIterateRemove( taskqueue_t *tq,
    int  (*cond)( lpel_task_t*,void*),
    void (*action)(lpel_task_t*,void*),
    void *arg );

void LpelTaskqueueDestroy(taskqueue_t *tq);

lpel_task_t *LpelTaskqueuePeek( taskqueue_t *tq);
int LpelTaskqueueSize(taskqueue_t *tq);

/* this function is used for HRC only, declared here to avoid separating header file for DECEN and HRC */
void LpelTaskqueueUpdatePriority(taskqueue_t *tq, lpel_task_t *t, double np);

/* these functions are implemented by DECEN but not used so far
void LpelTaskqueuePushFront( taskqueue_t *tq, lpel_task_t *t);
lpel_task_t *LpelTaskqueuePopBack(  taskqueue_t *tq);
*/
#endif /* _TASKQUEUE_H_ */
