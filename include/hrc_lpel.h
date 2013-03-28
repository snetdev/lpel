#ifndef _HRC_LPEL_H_
#define _HRC_LPEL_H_

#include <lpel.h>



/******************************************************************************/
/*  GENERAL CONFIGURATION AND SETUP                                           */
/******************************************************************************/
#define LPEL_MAP_OTHERS		-1
#define LPEL_MAP_MASTER		0


#define  TASK_INQUEUE							'Q'
#define  TASK_RETURNED						'T'

void LpelTaskSetPriorFunc(int func);
void LpelTaskSetRecLimit(lpel_task_t *t, int lim);
#endif /* _HRC_LPEL_H */
