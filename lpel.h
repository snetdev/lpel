
#ifndef _LPEL_H_
#define _LPEL_H_

#include "task.h"


typedef struct {
  int num_workers;
  int proc_workers;
  int proc_others;
  int flags;
} lpelconfig_t;

#define LPEL_FLAG_AUTO     (1<<0)
#define LPEL_FLAG_REALTIME (1<<1)


extern void LpelInit(lpelconfig_t *cfg);
extern void LpelRun(void);
extern void LpelCleanup(void);


extern int LpelNumWorkers(void);

extern void LpelTaskAdd(task_t *t);
extern void LpelTaskRemove(task_t *t);

#endif /* _LPEL_H_ */
