#ifndef _MONITORING_H_
#define _MONITORING_H_

#include <stdio.h>



#define MON_MAX_EVENTS  10

#define MONITORING_TIMES        (1<<0)
#define MONITORING_STREAMINFO   (1<<1)

#define MONITORING_ALLTASKS     (1<<8)

#define MONITORING_NONE           ( 0)
#define MONITORING_ALL            (-1)

typedef struct {
  FILE *outfile;
  int   flags;
  int   num_events;
  char  events[MON_MAX_EVENTS][20];
} monitoring_t;

#include "lpel.h"

void MonInit( lpelthread_t *env, int flags);
void MonCleanup( lpelthread_t *env);
void MonDebug( lpelthread_t *env, const char *fmt, ...);

struct task;

void MonTaskEvent( struct task *t, const char *fmt, ...);
void MonTaskPrint( struct task *t);




#endif /* _MONITORING_H_ */
