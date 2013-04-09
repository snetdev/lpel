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


/* set priority for task
 * prio: integer as it is used as index of the task queue
 */
void LpelTaskSetPriority(lpel_task_t *t, int prio);


/******************************************************************************/
/*  SPMD FUNCTIONS                                                            */
/******************************************************************************/
typedef void (*lpel_spmdfunc_t)(void *);
int LpelSpmdVId(void);

/** enter SPMD request */
void LpelTaskEnterSPMD(lpel_spmdfunc_t, void *);



#endif /* _DECEN_LPEL_H */
