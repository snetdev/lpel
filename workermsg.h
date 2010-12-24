#ifndef _WORKERMSG_H_
#define _WORKERMSG_H_


struct task;

typedef enum {
  WORKER_MSG_TERMINATE,
  WORKER_MSG_WAKEUP,
  WORKER_MSG_ASSIGN,
} workermsg_type_t;

typedef struct {
  workermsg_type_t type;
  struct task *task;
} workermsg_t;



#endif /* _WORKERMSG_H_ */

