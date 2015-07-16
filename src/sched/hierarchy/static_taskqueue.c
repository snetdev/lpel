/**********************************************************
 * Author: 	Nga
 *
 * Desc:		Static Task queue:
 * 					Used when task priority is static (never change). Tasks are stored in order of their priority, head is the highest one.
 * 					Tasks are not removed from the queue in their lifetime
 * 					When destroy task, remember to clean up = physically remove task out of the queue
 **********************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "lpelcfg.h"
#include "hrc_lpel.h"
#include "hrc_taskqueue.h"
#include "hrc_task.h"

#ifdef USE_STATIC_QUEUE

/**
 * A simple doubly linked list for task queues
 */

struct taskqueue_t {
	lpel_task_t *head;
	unsigned int count;
};


double comparePrior(lpel_task_t *t1, lpel_task_t *t2) {
	if (PRIO_CFG(rts_prio_cmp) == NULL)
		return (t1->sched_info.prio - t2->sched_info.prio);

	assert(t1->sched_info.rts_prio && t2->sched_info.rts_prio);
	return (double)PRIO_CFG(rts_prio_cmp)(t1->sched_info.rts_prio, t2->sched_info.rts_prio);
}


/**
 * Initialise a taskqueue
 */
taskqueue_t* LpelTaskqueueInit()
{
	taskqueue_t *tq = (taskqueue_t *)malloc(sizeof(taskqueue_t));
	tq->head = NULL;
	tq->count = 0;
	return tq;
}


void LpelTaskqueuePush(taskqueue_t *tq, lpel_task_t *t)
{
	assert(t->prev == NULL && t->next == NULL);

	tq->count++;
	if (tq->head == NULL) {
		tq->head = t;
		/* t->prev = NULL; t->next = NULL; is precond */
		return;
	}

	if (comparePrior(t, tq->head) >= 0) { // insert by the beginning
		tq->head->prev = t;
		t->next = tq->head;
		tq->head = t;
		/* t->prev = NULL; is precond */
		return;
	}

	lpel_task_t *cur = tq->head;
	while (cur->next != NULL) {
		if (comparePrior(t, cur->next) >= 0) {	// insert t after cur
			cur->next->prev = t;
			t->next = cur->next;
			cur->next = t;
			t->prev = cur;
			return;
		}
		cur = cur->next;
	}

	// insert by the end
	cur->next = t;
	t->prev = cur;
}


/**
 * Return the task with highest priority
 * 	While searching, update the validity of the task
 *
 * @return NULL if taskqueue is empty or all tasks are invalid
 */
lpel_task_t *LpelTaskqueuePop(taskqueue_t *tq)
{
	tq->count--;
	if (tq->head == NULL)
		return NULL;

	lpel_task_t *t = tq->head;
	tq->head = t->next;
	if (tq->head != NULL)
		tq->head->prev = NULL;

	t->next = NULL;
	return t;
}

void LpelTaskqueueOccupyTask(taskqueue_t *tq, lpel_task_t *t)
{
	assert(t->next != NULL || t->prev != NULL || tq->head != NULL);
	tq->count--;

	lpel_task_t *prev = t->prev;
	lpel_task_t *next = t->next;
	if (prev == NULL) {	// head
		if (next == NULL)
			tq->head = NULL;	// queue is empty now
		else {
			tq->head = next;
			next->prev = NULL;
		}
		t->next = NULL;
		return;
	}

	prev->next = next;
	if (next != NULL)
		next->prev = prev;
	t->next = NULL;
	t->prev = NULL;
}

void LpelTaskqueueDestroy(taskqueue_t *tq){
	assert(tq->head == NULL && tq->count == 0);
	free(tq);
}

/*
 * Search for valid task with highest priority
 */
lpel_task_t *LpelTaskqueuePeek( taskqueue_t *tq) {
	lpel_task_t *t;
	if ( tq->head == NULL ) return NULL;

	t = tq->head;
	while (t != NULL) {
		if (LpelTaskUpdateValid(t) == 1)
			return t;
		t = t->next;
	}
	return NULL;
}


int LpelTaskqueueSize(taskqueue_t *tq) {
	return tq->count;
}

/*
 * empty function: as task priority is static
 */
void LpelTaskqueueUpdatePriority(taskqueue_t *tq, lpel_task_t *t, double np) {
	assert(0);		// should not be called
}




#endif /* USE_STATIC_QUEUE */
