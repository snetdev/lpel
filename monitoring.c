
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include "streamset.h"

#include "monitoring.h"
#include "timing.h"

#define IS_FLAG(v)  ( (mon->flags & (v)) == (v) )


void MonitoringInit(monitoring_t *mon, int worker_id, int flags)
{
  char fname[11];
  int cnt_written;
  cnt_written = snprintf(fname, 11, "out%03d.log", worker_id);
  assert(cnt_written==10);

  /* copy flags */
  mon->flags = flags;

  /* open logfile */
  mon->outfile = fopen(fname, "w");
  assert( mon->outfile != NULL);
}

void MonitoringCleanup(monitoring_t *mon)
{
  int ret;
  ret = fclose(mon->outfile);
  assert(ret == 0);
}

void MonitoringPrint(monitoring_t *mon, task_t *t)
{
  int ret;
  timing_t ts;

  if ( mon->flags == MONITORING_NONE ) return;

  TimingStart(&ts);

  fprintf( mon->outfile,
      "%lu%09lu "
      "wid %d tid %lu st %d disp %lu ",
      (unsigned long) ts.tv_sec, ts.tv_nsec,
      t->owner, t->uid, t->state, t->cnt_dispatch
      );

  if ( IS_FLAG( MONITORING_EXECTIMES ) ) {
    fprintf( mon->outfile,
        "last %f total %f eavg %f ",
        TimingToMSec(&t->time_lastrun), TimingToMSec(&t->time_totalrun), TimingToMSec(&t->time_expavg)
        );
  }
 
  if ( IS_FLAG( MONITORING_STREAMINFO ) ) {
    fprintf(mon->outfile, "W");
    StreamsetPrint( &t->streams_write, mon->outfile);
    fprintf(mon->outfile, "R");
    StreamsetPrint( &t->streams_read, mon->outfile);
  }

  fprintf(mon->outfile, "\n");
  ret = fflush(mon->outfile);
  assert(ret == 0);
}

void MonitoringDebug(monitoring_t *mon, const char *fmt, ...)
{
  timing_t ts;
  va_list ap;

  TimingStart(&ts);
  fprintf(mon->outfile, "%lu%09lu ", (unsigned long) ts.tv_sec, ts.tv_nsec);

  va_start(ap, fmt);
  vfprintf(mon->outfile, fmt, ap);
  va_end(ap);

  fflush(mon->outfile);
}
