/******************************************************************************
 * LPEL LIBRARY INTERFACE
 *
 * Author: Nga
 *
 * Extra interface for HRC
 *****************************************************************************/

#ifndef _HRC_LPEL_H_
#define _HRC_LPEL_H_

#include <lpel.h>


/* LPEL MAPPING LOCATION */
#define LPEL_MAP_OTHERS		-1
#define LPEL_MAP_MASTER		0


/* extra task state in HRC */
#define  TASK_INQUEUE							'Q'
#define  TASK_RETURNED						'T'

/* choose function to calculate task priority */
void LpelTaskSetPriorityFunc(int func);

/* set the limit of output records for a task */
void LpelTaskSetRecLimit(lpel_task_t *t, int lim);

#endif /* _HRC_LPEL_H */
