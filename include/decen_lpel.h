#ifndef _DECEN_LPEL_H_
#define _DECEN_LPEL_H_

#include <lpel.h>



/******************************************************************************/
/*  GENERAL CONFIGURATION AND SETUP                                           */
/******************************************************************************/
#define LPEL_MAP_OTHERS		-1

#define  TASK_CREATED             'C'
#define  TASK_RUNNING             'U'
#define  TASK_READY               'R'
#define  TASK_BLOCKED             'B'
#define  TASK_MUTEX               'X'
#define  TASK_ZOMBIE              'Z'


/** spmd function */
typedef void (*lpel_spmdfunc_t)(void *);


void LpelTaskSetPriority(lpel_task_t *t, int prio);

/******************************************************************************/
/*  SPMD FUNCTIONS                                                            */
/******************************************************************************/

int LpelSpmdVId(void);

/** enter SPMD request */
void LpelTaskEnterSPMD(lpel_spmdfunc_t, void *);



#endif /* _DECEN_LPEL_H */
