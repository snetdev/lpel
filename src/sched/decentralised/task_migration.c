#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "task.h"
#include "decen_worker.h"
#include "lpel.h"
#include "lpelcfg.h"
#include "task_migration.h"

lpel_tm_config_t tm_conf;

/* function to check if task should be migrated. 1 = yes, 0 = no*/
static int (*check_migrate_func)(lpel_task_t *) = NULL;

/* function to pick worker to migrate the task */
static int (*pick_worker_func) () = NULL;

/******* PRIVATE FUNCTION *******/
/* check by random */
int MigrateRandom(lpel_task_t *t) {
	double c = (double) rand() / (double) RAND_MAX;
	if (c > tm_conf.threshold)
		return 1;
	else
		return 0;
}

/* pick random worker */
int PickWorkerRandom() {
	return rand() % tm_conf.num_workers;
}


/*--------------------------------*/
/* migrate based on the waiting proportion */
int MigrateTaskWait(lpel_task_t *t) {
	if (!t->mon)
		return 0;
	double task_wait = MON_CB(get_task_wait_prop) (t->mon);
	double worker_wait = MON_CB(get_worker_wait_prop)(t->mon);
	double global_wait = MON_CB(get_global_wait_prop)();
	if (task_wait > worker_wait && worker_wait > global_wait)
		return 1;
	else
		return 0;
}

/* choose the worker with the most wait proportion */
int PickWorkerTaskWait() {
	return MON_CB(worker_most_wait_prop)();
}


/***************************************/


/*
 * Initialize migration mechanism
 */
void LpelTaskMigrationInit(lpel_tm_config_t *conf) {
	tm_conf = *conf;
	switch(tm_conf.mechanism) {
	case LPEL_MIG_RAND:
		check_migrate_func = MigrateRandom;
		pick_worker_func = PickWorkerRandom;
		break;
	case LPEL_MIG_WAIT_PROP:
		/* check if callback functions in monitoring framework are set */
		if (MON_CB(get_task_wait_prop) && MON_CB(get_worker_wait_prop)
				&& MON_CB(get_global_wait_prop) && MON_CB(worker_most_wait_prop)
				&& MON_CB(task_assign) && MON_CB(task_destroy)
				&& MON_CB(task_start) && MON_CB(task_stop)) {
			check_migrate_func = MigrateTaskWait;
			pick_worker_func = PickWorkerTaskWait;
		}
		break;
	}
}

/*
 * pick target worker to migrate the task
 * @param t			task
 * @return wid	worker id
 * 							If wid < 0 --> should not migrate the task
 */
int LpelPickTargetWorker(lpel_task_t *t) {
	if (check_migrate_func && pick_worker_func)
		if (check_migrate_func(t))
			return pick_worker_func();
	return -1;
}

