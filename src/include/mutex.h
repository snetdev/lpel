#ifndef _MUTEX_H_
#define _MUTEX_H_


#include "task.h"

typedef struct {
  volatile int counter;
  unsigned char padding[64-sizeof(int)];
} lpel_mutex_t;


void LpelMutexInit(lpel_mutex_t *mx);
void LpelMutexDestroy(lpel_mutex_t *mx);
void LpelMutexEnter(lpel_task_t *t, lpel_mutex_t *mx);
void LpelMutexLeave(lpel_task_t *t, lpel_mutex_t *mx);


#endif /* _MUTEX_H_ */
