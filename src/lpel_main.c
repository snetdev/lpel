/**
 * Main LPEL module
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <errno.h>
#include <sched.h>
#include <unistd.h>  /* sysconf() */
#include <pthread.h> /* worker threads are OS threads */

#include <lpel.h>

#include "arch/mctx.h"
#include "lpel_hwloc.h"
#include "lpelcfg.h"
#include "worker.h"


/**
 * Get the number of available cores
 */
int LpelGetNumCores( int *result)
{
  int proc_avail = -1;
#ifdef HAVE_SYSCONF
  /* query the number of CPUs */
  proc_avail = sysconf(_SC_NPROCESSORS_ONLN);
#endif
  if (proc_avail == -1) {
      char *p = getenv("LPEL_NUM_WORKERS");
      if (p != 0)
      {
          unsigned long n = strtoul(p, 0, 0);
          if (errno != EINVAL)
              proc_avail = n;
      }
  }
  if (proc_avail == -1) {
    return LPEL_ERR_FAIL;
  }
  *result = proc_avail;
  return 0;
}


/**
 * Initialise the LPEL
 *
 *  num_workers, proc_workers > 0
 *  proc_others >= 0
 *
 *
 * EXCLUSIVE: only valid, if
 *       #proc_avail >= proc_workers + proc_others &&
 *       proc_others != 0 &&
 *       num_workers == proc_workers
 *
 */
void LpelInit(lpel_config_t *cfg)
{
  /* Initialise hardware information for thread pinning */
  LpelHwLocInit(cfg);

#ifdef USE_MCTX_PCL
  int res = co_thread_init();
  /* initialize machine context for main thread */
  assert( 0 == res);
#endif
}


int LpelStart(lpel_config_t *cfg)
{
  int res;

  /* store a local copy of cfg */
  _lpel_global_config = *cfg;

  /* check the config */
  res = LpelHwLocCheckConfig(cfg);
  if (res) return res;

  LpelHwLocStart(cfg);

  /* initialise workers */
  LpelWorkersInit( _lpel_global_config.num_workers);

  LpelWorkersSpawn();

  return 0;
}

void LpelStop(void)
{
  LpelWorkersTerminate();
}



/**
 * Cleans the LPEL up
 * - wait for the workers to finish
 * - free the data structures of worker threads
 */
void LpelCleanup(void)
{
  /* Cleanup workers */
  LpelWorkersCleanup();

  /* Cleanup hardware info */
  LpelHwLocCleanup();

#ifdef USE_MCTX_PCL
  /* cleanup machine context for main thread */
  co_thread_cleanup();
#endif
}





