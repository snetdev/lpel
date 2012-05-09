#include <stdlib.h>
#include "oracle.h"

void LpelRunOracle(lpel_task_iterator_t *iter, int number_of_workers)
{
  while(LpelTaskIterHasNext(iter)) {
    lpel_task_t *t;
    int current_worker;
    double c;
    int migrate;

    t = LpelTaskIterNext(iter);
    current_worker = t->current_worker;

    c = (double)rand() / (double)RAND_MAX;
    migrate = (c < 0.5) ? 1 : 0;
    t->new_worker = c ? rand() % number_of_workers : current_worker;
  }
  LpelTaskIterDestroy(iter);
}
