#ifndef _MONITORING_H_
#define _MONITORING_H_



#define LPEL_MON_TASK_TIMES   (1<<0)
#define LPEL_MON_TASK_STREAMS (1<<1)

#include <lpel.h>

struct mon_worker_t;
struct mon_task_t;
struct mon_stream_t;

void LpelMonInit(lpel_monitoring_cb_t *cb);
void LpelMonCleanup(void);


struct mon_task_t *LpelMonTaskCreate(unsigned long tid,
    const char *name, unsigned long flags);




#endif /* _MONITORING_H_ */
