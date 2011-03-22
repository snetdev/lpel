
#ifndef _THREADING_H_
#define _THREADING_H_

#define LPEL_USE_CAPABILITIES


#include <pthread.h>

/******************************************************************************/
/*  RETURN VALUES OF LPEL FUNCTIONS                                           */
/******************************************************************************/

#define LPEL_ERR_SUCCESS     0
#define LPEL_ERR_FAIL       -1

#define LPEL_ERR_INVAL       1 /* Invalid argument */
#define LPEL_ERR_ASSIGN      2 /* Cannot assign thread to processors */
#define LPEL_ERR_EXCL        3 /* Cannot assign core exclusively */

/******************************************************************************/
/*  GENERAL CONFIGURATION AND SETUP                                           */
/******************************************************************************/

/**
 * Specification for configuration:
 *
 * num_workers defines the number of worker threads (PThreads) spawned.
 * proc_workers is the number of processors used for workers.
 * proc_others is the number of processors assigned to other than
 *   worker threads.
 * flags:
 *   AUTO - use default setting for num_workers, proc_workers, proc_others
 *   REALTIME - set realtime priority for workers, will succeed only if
 *              there is a 1:1 mapping of workers to procs,
 *              proc_others > 0 and the process has needed privileges.
 */
typedef struct {
  int num_workers;
  int proc_workers;
  int proc_others;
  int flags;
  int node;
  int worker_dbg;
} lpel_config_t;


#define LPEL_FLAG_NONE           (0)
#define LPEL_FLAG_PINNED      (1<<0)
#define LPEL_FLAG_EXCLUSIVE   (1<<1)



int LpelInit( lpel_config_t *cfg);
void LpelCleanup( void);

void LpelStop(void);

int LpelGetNumCores( int *result);
int LpelCanSetExclusive( int *result);
int LpelThreadAssign( int core);


extern lpel_config_t    _lpel_global_config;

#endif /* _THREADING_H_ */
