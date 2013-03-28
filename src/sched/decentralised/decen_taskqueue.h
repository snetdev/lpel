#ifndef _DECEN_TASKQUEUE_H_
#define _DECEN_TASKQUEUE_H_

#include "taskqueue.h"
#include "task.h"


struct taskqueue_t {
  lpel_task_t *head, *tail;
  unsigned int count;
};

#endif 	/* _DECEN_TASKQUEUE_H */

