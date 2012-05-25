#include <stdio.h>
#include "taskiterator.h"

static void FindNext(lpel_task_iterator_t *iter) {
  iter->next = (iter->current != NULL) ? iter->current->next : NULL;
  while( iter->next == NULL && iter->index-- >= 0) {
    iter->next = iter->queue[iter->index].head;
  }
}

lpel_task_iterator_t * LpelTaskIterCreate(taskqueue_t *queue,
                                          int length)
{
  int i = 0;
  lpel_task_iterator_t *iter = malloc(sizeof(lpel_task_iterator_t));
  iter->queue = queue;
  iter->current = NULL;

  iter->index = length-1;

  iter->queue_length = length;


  do {
    iter->current = queue[iter->index].head;
  } while(iter->current == NULL && iter->index-- >= 0);



  FindNext(iter);

  return iter;
}

lpel_task_iterator_t * LpelTaskIterDestroy(lpel_task_iterator_t *iter)
{
  free(iter);
}

int LpelTaskIterHasNext(lpel_task_iterator_t *iter)
{
  return iter->current != NULL &&
         iter->index >= 0 &&
         iter->index < iter->queue_length;
}

lpel_task_t * LpelTaskIterNext(lpel_task_iterator_t *iter)
{
  lpel_task_t *t = iter->current;
  iter->current = (iter->current != NULL) ? iter->next : NULL;
  FindNext(iter);
  return t;
}
