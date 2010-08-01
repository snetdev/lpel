#ifndef _INITQUEUE_H_
#define _INITQUEUE_H_

#include <pthread.h>
#include "task.h"


typedef struct initqueue initqueue_t;

extern initqueue_t *InitqueueCreate(void);
extern void InitqueueDestroy(initqueue_t *iq);
extern void InitqueueEnqueue(initqueue_t *iq, task_t *t);
extern task_t *InitqueueDequeue(initqueue_t *iq);



#endif /* _INITQUEUE_H_ */
