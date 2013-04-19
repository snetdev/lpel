/******************************************************************************
 * LPEL LIBRARY INTERFACE
 *
 * Author: Nga
 *
 * Extra interface for DECEN
 *****************************************************************************/

#ifndef _DECEN_LPEL_H_
#define _DECEN_LPEL_H_

#include <lpel.h>

/* task migration mechanism */
typedef enum {
	LPEL_MIG_NONE,
	LPEL_MIG_RAND,
	LPEL_MIG_WAIT_PROP,
} lpel_tm_mechanism;

/* task migration configuration */
typedef struct {
  double threshold;
  int num_workers;
  lpel_tm_mechanism mechanism;
} lpel_tm_config_t;


/* set priority for task
 * prio: integer as it is used as index of the task queue
 */
void LpelTaskSetPriority(lpel_task_t *t, int prio);

/* get wid of a task */
int LpelTaskGetWorkerId(lpel_task_t *t);

/******************************************************************************/
/*  SPMD FUNCTIONS                                                            */
/******************************************************************************/
typedef void (*lpel_spmdfunc_t)(void *);
int LpelSpmdVId(void);

/** enter SPMD request */
void LpelTaskEnterSPMD(lpel_spmdfunc_t, void *);

void LpelTaskMigrationInit(lpel_tm_config_t *conf);


#endif /* _DECEN_LPEL_H */
