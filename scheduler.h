
#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include <pthread.h>


typedef struct stream stream_t;



typedef struct {
  //coroutine_t component;
  stream_t *input;
  stream_t *output;
} task_t;

// create task
// create IOtask



#endif /* _SCHEDULER_H_ */
