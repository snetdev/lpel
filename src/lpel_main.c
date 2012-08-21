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
//#include "lpel_main.h"
#include "lpelcfg.h"
#include "worker.h"
#include "lpel_hwloc.h"


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
  memset(cfg, 0, sizeof(lpel_config_t));

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
  LpelWorkerInit( _lpel_global_config.num_workers);

  LpelWorkerSpawn();

  if (cfg->placement) {
    /* Initialize placement scheduler */
    LpelPlacementSchedulerInit();
  }

  return 0;
}


void LpelStop(void)
{
  LpelWorkerTerminate();
}



/**
 * Cleans the LPEL up
 * - wait for the workers to finish
 * - free the data structures of worker threads
 */
void LpelCleanup(void)
{
  /* Cleanup workers */
  LpelWorkerCleanup();

  /* Cleanup hardware info */
  LpelHwLocCleanup();

#ifdef USE_MCTX_PCL
  /* cleanup machine context for main thread */
  co_thread_cleanup();
#endif
}
