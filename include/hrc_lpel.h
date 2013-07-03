/******************************************************************************
 * LPEL LIBRARY INTERFACE
 *
 * Author: Nga
 *
 * Extra interface for HRC
 *****************************************************************************/

#ifndef _HRC_LPEL_H_
#define _HRC_LPEL_H_

#include <lpel_common.h>


/* extra task state in HRC */
#define  TASK_INQUEUE							'Q'
#define  TASK_RETURNED						'T'

/* choose function to calculate task priority */
void LpelTaskSetPriorityFunc(int func);

/* set negative demand limit */
void LpelTaskSetNegLim(int lim);

/* set the limit of output records for a task */
void LpelTaskSetRecLimit(lpel_task_t *t, int lim);

#endif /* _HRC_LPEL_H */
