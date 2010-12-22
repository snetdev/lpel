#ifndef _MONITORING_H_
#define _MONITORING_H_


#define MONITORING_ENABLE


#ifdef MONITORING_ENABLE

#include <stdio.h>

typedef struct monitoring monitoring_t;


struct task;


monitoring_t *MonitoringCreate(int node, char *name);
void MonitoringDestroy( monitoring_t *mon);


void MonitoringDebug( monitoring_t *mon, const char *fmt, ...);

void MonitoringOutput( monitoring_t *mon, struct task *t);


#endif /* MONITORING_ENABLE */

#endif /* _MONITORING_H_ */
