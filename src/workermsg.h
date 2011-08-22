#ifndef _WORKERMSG_H_
#define _WORKERMSG_H_

#include "task.h"

/*
 * worker msg body
 */
typedef enum {
  WORKER_MSG_TERMINATE = 1,
  WORKER_MSG_WAKEUP,
  WORKER_MSG_ASSIGN,
  WORKER_MSG_WORLDREQ,
  WORKER_MSG_REQUEST,
  WORKER_MSG_TASKMIG,
} workermsg_type_t;

typedef struct {
  workermsg_type_t  type;
  union {
    lpel_task_t    *task;
    int            from_worker;
  } body;
} workermsg_t;

#endif /* _WORKERMSG_H_ */
