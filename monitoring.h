#ifndef _MONITORING_H_
#define _MONITORING_H_


#include "task.h"


typedef struct monitoring monitoring_t;


extern void MonitoringInit(monitoring_t **mon, int worker_id);
extern void MonitoringCleanup(monitoring_t *mon);
extern void MonitoringPrint(monitoring_t *mon, task_t *t);
extern void MonitoringDebug(monitoring_t *mon, const char *fmt, ...);




#endif /* _MONITORING_H_ */
