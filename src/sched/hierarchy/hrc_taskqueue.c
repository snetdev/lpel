/**********************************************************
 * Author: 	Nga
 *
 * Desc:		Priority task queue
 * 			Implemented by a heap structure
 * 			Tasks are stored in an array initialsed with fix size.
 * 			When the array is full, it is reallocated.
 **********************************************************/


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

/******************** private functions ********************/
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

/**************************************************************/

/*
 * Initialise a task queue
 */
taskqueue_t* LpelTaskqueueInit() {
  taskqueue_t *tq = (taskqueue_t *)malloc(sizeof(taskqueue_t));
  tq->alloc = BLOCKSIZE;
  tq->count = 1;  /* first element in array is not used to simplify indices */
  tq->heap = (lpel_task_t **) malloc(tq->alloc * sizeof(lpel_task_t *));
  return tq;
}

/*
 * Add a task to the task queue
 */
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


/*
 * pop the task with highest priority
 */
lpel_task_t *LpelTaskqueuePop( taskqueue_t *tq) {
  {
    if (tq->count == 1)
      return NULL;

    lpel_task_t *t = tq->heap[1];
    LpelTaskqueueRemoveHead(tq);
    return t;
  }
}

/*
 * Get queue size
 */
int LpelTaskqueueSize(taskqueue_t *tq){
  return tq->count - 1;
}

/*
 * Free memory of the task queue
 */
void LpelTaskqueueDestroy(taskqueue_t *tq){
  free (tq->heap);
  free(tq);
}

/*
 * Update priority for a task in the queue
 */
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

