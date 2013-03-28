#include <stdlib.h>
#include <assert.h>

#include "taskqueue.h"
#include "hrc_task.h"

#define BLOCKSIZE 50

struct taskqueue_t{
  lpel_task_t **heap;
  unsigned int count;
  unsigned int alloc;
};


void upHeap(taskqueue_t *tq, int pos) {
	int parent;
	lpel_task_t *t = tq->heap[pos];
	double p = t->sched_info->prior;
	  /* append at end, then up heap */
	  while ((parent = pos / 2) &&  p > tq->heap[parent]->sched_info->prior) {
	    tq->heap[pos] = tq->heap[parent];
	    pos = parent;
	  }
	  tq->heap[pos] = t;
}

void downHeap(taskqueue_t *tq, int pos) {
	  int child;
		lpel_task_t *t = tq->heap[pos];
		double p = t->sched_info->prior;
		while( (child = pos * 2) < tq->count) {
			if (child + 1 < tq->count && tq->heap[child]->sched_info->prior < tq->heap[child+1]->sched_info->prior)
				child++;

			if (p >= tq->heap[child]->sched_info->prior)
				break;

			tq->heap[pos] = tq->heap[child];
			pos = child;
		}
		tq->heap[pos] = t;

}


int searchItem(taskqueue_t *tq, lpel_task_t *t) {
	int i;
	for (i = 1; i < tq->count; i++) {
		if (tq->heap[i] == t)
			return i;
	}
	return -1;
}

taskqueue_t* LpelTaskqueueInit() {
  taskqueue_t *tq = (taskqueue_t *)malloc(sizeof(taskqueue_t));
  tq->alloc = BLOCKSIZE;
  tq->count = 1;  /* first element in array is not used to simplify indices */
  tq->heap = (lpel_task_t **) malloc(tq->alloc * sizeof(lpel_task_t *));
  return tq;
}

void LpelTaskqueuePush( taskqueue_t *tq, lpel_task_t *t){

  //allocate more memory if needed
  if (tq->count >= tq->alloc) {
    tq->alloc = tq->alloc + BLOCKSIZE;
    tq->heap = realloc(tq->heap, tq->alloc *sizeof(taskqueue_t*));
  }

  tq->heap[tq->count] = t;
  upHeap(tq, tq->count);
  tq->count++;
}


/*
 * remove the head (highest priority) of the queue
 */
void LpelTaskqueueRemoveHead(taskqueue_t *tq){
  /* pull last item to top, then down heap. */
  tq->count--;
  tq->heap[1] = tq->heap[tq->count];
  downHeap(tq, 1);
}

/*
 * retrieve head (highest priority) of the queue
 */
lpel_task_t *LpelTaskqueuePeek( taskqueue_t *tq){
  if (tq->count == 1)
    return NULL;

  return tq->heap[1];
}



lpel_task_t *LpelTaskqueuePop( taskqueue_t *tq) {
  {
    if (tq->count == 1)
      return NULL;

    lpel_task_t *t = tq->heap[1];
    LpelTaskqueueRemoveHead(tq);
    return t;
  }
}

int LpelTaskqueueSize(taskqueue_t *tq){
  return tq->count - 1;
}

void LpelTaskqueueDestroy(taskqueue_t *tq){
  free (tq->heap);
  free(tq);
}

void LpelTaskqueueUpdatePriority(taskqueue_t *tq, lpel_task_t *t, double np){
	int pos = searchItem(tq, t);
	assert(pos > 0);
	double p = t->sched_info->prior;
	t->sched_info->prior = np;
	if (np > p)
		upHeap(tq, pos);
	else
		downHeap(tq, pos);
}

