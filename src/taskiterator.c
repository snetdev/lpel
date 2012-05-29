#include <stdio.h>
#include "taskiterator.h"

static lpel_task_iterator_t iter;

static void FindNext() {
  iter.next = (iter.current != NULL) ? iter.current->next : NULL;
  while( iter.next == NULL && iter.index-- >= 0) {
    iter.next = iter.queue[iter.index].head;
  }
}

void LpelTaskIterReset(taskqueue_t *queue, int length)
{
  int i = 0;
  iter.queue = queue;
  iter.current = NULL;
  iter.next = NULL;

  iter.index = length-1;

  iter.queue_length = length;


  do {
    iter.current = queue[iter.index].head;
  } while(iter.current == NULL && iter.index-- >= 0);



  FindNext();
}

int LpelTaskIterHasNext()
{
  return iter.current != NULL &&
         iter.index >= 0 &&
         iter.index < iter.queue_length;
}

lpel_task_t * LpelTaskIterNext()
{
  lpel_task_t *t = iter.current;
  iter.current = (iter.current != NULL) ? iter.next : NULL;
  FindNext();
  return t;
}
