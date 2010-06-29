
#ifndef _LPEL_H_
#define _LPEL_H_

#include "task.h"


typedef struct {
  int num_workers;
  unsigned long attr;
} lpelconfig_t;

typedef enum {
  LPEL_ATTR_ASSIGNCORE = (1<<0)
} lpelconfig_attr_t;


extern void LpelInit(lpelconfig_t *cfg);
extern void LpelRun(void);
extern void LpelCleanup(void);


extern int LpelGetWorkerId(void);
extern task_t *LpelGetCurrentTask(void);


#endif /* _LPEL_H_ */
