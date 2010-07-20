#ifndef _MONITORING_H_
#define _MONITORING_H_


#include "task.h"


typedef struct monitoring monitoring_t;


extern void MonitoringInit(monitoring_t **mon, int worker_id);
extern void MonitoringPrint(monitoring_t *mon, task_t *t);
extern void MonitoringCleanup(monitoring_t *mon);




#endif /* _MONITORING_H_ */
