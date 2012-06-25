#ifndef _TASK_ITERATOR_H_
#define _TASK_ITERATOR_H_

#include <lpel.h>
#include "taskqueue.h"

struct lpel_task_iterator {
  lpel_task_t *current;
  lpel_task_t *next;
  taskqueue_t * queue;
  int queue_length;
  int index;
  int order;
};

int LpelTaskIterHasNext(lpel_task_iterator_t *iter);
lpel_task_t * LpelTaskIterNext(lpel_task_iterator_t *iter);
lpel_task_iterator_t * LpelTaskIterDestroy(lpel_task_iterator_t *iter);
#endif
