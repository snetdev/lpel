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

/* define use of negative demand limit */
#define _USE_NEG_DEMAND_LIMIT_


/* extra task state in HRC */
#define  TASK_INQUEUE							'Q'
#define  TASK_RETURNED						'T'

lpel_task_t *LpelTaskCreate( int worker, lpel_taskfunc_t func,
    void *inarg, int stacksize, void *rts_prio );

/* initialise priority configuration */
void LpelTaskPrioInit(lpel_task_prio_conf *conf) ;

/* set the limit of output records for a task */
void LpelTaskSetRecLimit(lpel_task_t *t, int lim);

#endif /* _HRC_LPEL_H */
