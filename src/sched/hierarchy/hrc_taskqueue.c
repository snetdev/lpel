/**********************************************************
 * Author: 	Nga
 *
 * Desc:		Priority task queue
 * 			Implemented by a heap structure
 * 			Tasks are stored in an array initialsed with fix size.
 * 			When the array is full, it is reallocated.
 **********************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "lpelcfg.h"
#include "hrc_lpel.h"
#include "hrc_taskqueue.h"
#include "hrc_task.h"

#ifdef USE_HEAP_QUEUE

#define BLOCKSIZE 50

struct taskqueue_t{
  lpel_task_t **heap;
  unsigned int count;
  unsigned int alloc;
};


int comparePrior(lpel_task_t *t1, lpel_task_t *t2) {
	int valid = t1->sched_info.valid - t2->sched_info.valid;
	if (valid != 0)	/* one of the two task is invalid */
		return valid;
	else if (t1->sched_info.valid == 0)	/* both of the task are invalid, consider they equal */
		return 0;

	if (PRIO_CFG(rts_prio_cmp) == NULL)
		return (t1->sched_info.prio - t2->sched_info.prio);

	assert(t1->sched_info.rts_prio && t2->sched_info.rts_prio);
	return (double)PRIO_CFG(rts_prio_cmp)(t1->sched_info.rts_prio, t2->sched_info.rts_prio);
}

/******************** private functions ********************/
void upHeap(taskqueue_t *tq, int pos) {
	int parent;
	lpel_task_t *t = tq->heap[pos];
	  /* append at end, then up heap */
	  while ((parent = pos / 2) &&  comparePrior(t, tq->heap[parent]) > 0) {
	  	//p > tq->heap[parent]->sched_info.prio)
	    tq->heap[pos] = tq->heap[parent];
	    pos = parent;
	  }
	  tq->heap[pos] = t;
}

void downHeap(taskqueue_t *tq, int pos) {
	  int child;
		lpel_task_t *t = tq->heap[pos];
		while( (child = pos * 2) < tq->count) {
			if (child + 1 < tq->count && comparePrior(tq->heap[child], tq->heap[child + 1]) < 0)
					//tq->heap[child]->sched_info.prio < tq->heap[child+1]->sched_info.prio)
				child++;

			if (comparePrior(t, tq->heap[child]) >= 0)
			//if (p >= tq->heap[child]->sched_info.prio)
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


/*
 * remove the head (highest priority) of the queue
 */
void removeHead(taskqueue_t *tq){
  /* pull last item to top, then down heap. */
  tq->count--;
  tq->heap[1] = tq->heap[tq->count];
  downHeap(tq, 1);
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
  if (t->sched_info.valid == 1)		//otherwise, it should be in the last position
  	upHeap(tq, tq->count);
  tq->count++;
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
	if (tq->count == 1)
		return NULL;

	lpel_task_t *t = tq->heap[1];
	removeHead(tq);
	return t;
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
	double p = t->sched_info.prio;
	t->sched_info.prio = np;
	if (np < p || t->sched_info.valid == 0)
		downHeap(tq, pos);
	else if (np > p)
		upHeap(tq, pos);
}

/*
 * task is occupied, remove it from the queue
 *
 */
void LpelTaskqueueOccupyTask (taskqueue_t *tq, lpel_task_t *t) {
	int pos = searchItem(tq, t);
	assert(pos > 0);
	tq->count--;
	tq->heap[pos] = tq->heap[tq->count];
	downHeap(tq, 1);
}


#endif  /* USE_HEAP_QUEUE */
