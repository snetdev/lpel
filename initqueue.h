#ifndef _INITQUEUE_H_
#define _INITQUEUE_H_

#include <pthread.h>
#include "task.h"


typedef struct {
  task_t *head;
  task_t *tail;
  pthread_mutex_t lock_tail;
} initqueue_t;

extern void InitqueueInit(initqueue_t *iq);
extern void InitqueueCleanup(initqueue_t *iq);
extern void InitqueueEnqueue(initqueue_t *iq, task_t *t);
extern task_t *InitqueueDequeue(initqueue_t *iq);



#endif /* _INITQUEUE_H_ */
