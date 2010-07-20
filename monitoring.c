
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "monitoring.h"
#include "timing.h"

struct monitoring {
  FILE *outfile;
};


void MonitoringInit(monitoring_t **mon, int worker_id)
{
  char fname[11];
  int cnt_written;
  cnt_written = snprintf(fname, 11, "out%03d.log", worker_id);
  assert(cnt_written==10);

  *mon = (monitoring_t *) malloc( sizeof(monitoring_t) );
  (*mon)->outfile = fopen(fname, "w");
  assert((*mon)->outfile != NULL);
}

void MonitoringPrint(monitoring_t *mon, task_t *t)
{
  timing_t ts;
  TimingStart(&ts);

  fprintf(mon->outfile,
    "%lu%09lu "
    "wid %d tid %lu %d "
    "last %f total %f eavg %f "
    "\n",
    (unsigned long) ts.tv_sec, ts.tv_nsec,
    t->owner, t->uid, t->state,
    TimingToMSec(&t->time_lastrun), TimingToMSec(&t->time_totalrun), TimingToMSec(&t->time_expavg)
    );
}

void MonitoringCleanup(monitoring_t *mon)
{
  int ret;
  ret = fclose(mon->outfile);
  assert(ret == 0);

  free(mon);
}

