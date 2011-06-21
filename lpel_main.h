
#ifndef _LPEL_MAIN_H_
#define _LPEL_MAIN_H_

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

typedef struct mon_worker_t mon_worker_t;
typedef struct mon_task_t   mon_task_t;
typedef struct mon_stream_t mon_stream_t;

enum lpel_taskstate_t;

typedef struct {
  /* worker callbacks*/
  mon_worker_t *(*worker_create)(int);
  mon_worker_t *(*worker_create_wrapper)(mon_task_t *);
  void (*worker_destroy)(mon_worker_t*);
  void (*worker_waitstart)(mon_worker_t*);
  void (*worker_waitstop)(mon_worker_t*);
  /* task callbacks */
  /* note: no callback for task creation
   * - manual attachment required
   */
  void (*task_destroy)(mon_task_t*);
  void (*task_assign)(mon_task_t*, mon_worker_t*);
  void (*task_start)(mon_task_t*);
  void (*task_stop)(mon_task_t*, enum lpel_taskstate_t);
  /* stream callbacks */
  mon_stream_t *(*stream_open)(mon_task_t*, unsigned int, char);
  void (*stream_close)(mon_stream_t*);
  void (*stream_replace)(mon_stream_t*, unsigned int);
  void (*stream_readprepare)(mon_stream_t*);
  void (*stream_readfinish)(mon_stream_t*, void*);
  void (*stream_writeprepare)(mon_stream_t*, void*);
  void (*stream_writefinish)(mon_stream_t*);
  void (*stream_blockon)(mon_stream_t*);
  void (*stream_wakeup)(mon_stream_t*);
  /* general purpose debug */
  void (*debug)(mon_worker_t*, const char *fmt, ...);
} lpel_monitoring_cb_t;


#define _MON_CB_MEMBER(glob,member) (glob.member)
#define MON_CB(name) (_MON_CB_MEMBER(_lpel_global_config.mon,name))

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
  int worker_dbg;
  lpel_monitoring_cb_t mon;
} lpel_config_t;


enum lpel_taskstate_t {
  TASK_CREATED = 'C',
  TASK_RUNNING = 'U',
  TASK_READY   = 'R',
  TASK_BLOCKED = 'B',
  TASK_MUTEX   = 'X',
  TASK_ZOMBIE  = 'Z'
};




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

#endif /* _LPEL_MAIN_H_ */
