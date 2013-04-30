#ifndef _WORKERMSG_H_
#define _WORKERMSG_H_

#include "task.h"

/*
 * worker msg body
 */
typedef int workermsg_type_t;


typedef struct {
  workermsg_type_t  type;
  union {
    lpel_task_t    *task;
    int            from_worker;
  } body;
} workermsg_t;

#endif /* _WORKERMSG_H_ */
