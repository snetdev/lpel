#ifndef _LPELMAIN_H
#define _LPELMAIN_H

#include <pthread.h>
#include <lpel_common.h>

struct lpel_stream_desc_t {
  lpel_task_t   *task;        /** the task which opened the stream */
  lpel_stream_t *stream;      /** pointer to the stream */
  char mode;                  /** either 'r' or 'w' */
  struct lpel_stream_desc_t *next; /** for organizing in stream sets */
  struct mon_stream_t *mon;   /** monitoring object */
};

//#define STREAM_POLL_SPINLOCK

/** Macros for lock handling */

#ifdef STREAM_POLL_SPINLOCK

#define PRODLOCK_TYPE       pthread_spinlock_t
#define PRODLOCK_INIT(x)    pthread_spin_init(x, PTHREAD_PROCESS_PRIVATE)
#define PRODLOCK_DESTROY(x) pthread_spin_destroy(x)
#define PRODLOCK_LOCK(x)    pthread_spin_lock(x)
#define PRODLOCK_UNLOCK(x)  pthread_spin_unlock(x)

#else

#define PRODLOCK_TYPE       pthread_mutex_t
#define PRODLOCK_INIT(x)    pthread_mutex_init(x, NULL)
#define PRODLOCK_DESTROY(x) pthread_mutex_destroy(x)
#define PRODLOCK_LOCK(x)    pthread_mutex_lock(x)
#define PRODLOCK_UNLOCK(x)  pthread_mutex_unlock(x)

#endif /* STREAM_POLL_SPINLOCK */



void LpelWorkersInit( lpel_config_t *);
void LpelWorkersCleanup( void);
void LpelWorkersSpawn(void);
void LpelWorkersTerminate(void);


#endif /* _LPELMAIN_H */
