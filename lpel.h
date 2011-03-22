/**
 * MAIN LPEL INTERFACE
 */
#ifndef _LPEL_H_
#define _LPEL_H_



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




/******************************************************************************/
/*  DATATYPES                                                                 */
/******************************************************************************/

/* task types */

/* a task */
typedef struct lpel_task_t lpel_task_t;

/* task function signature */
typedef void (*lpel_taskfunc_t)( lpel_task_t *self, void *inarg);





/* stream types */

typedef struct lpel_stream_t         lpel_stream_t;

typedef struct lpel_stream_desc_t    lpel_stream_desc_t;    

typedef lpel_stream_desc_t          *lpel_streamset_t;

typedef struct lpel_stream_iter_t    lpel_stream_iter_t;


/* monitoring */

#define LPEL_MON_TASK_TIMES   (1<<0)
#define LPEL_MON_TASK_STREAMS (1<<1)



/******************************************************************************/
/*  TASK FUNCTIONS                                                            */
/******************************************************************************/

lpel_task_t *LpelTaskCreate( int worker, lpel_taskfunc_t func,
    void *inarg, int stacksize );

/** monitor a task */
void LpelTaskMonitor( lpel_task_t *t, char *name, unsigned long flags);

unsigned int LpelTaskGetUID( lpel_task_t *t );

/** let the previously created task run */
void LpelTaskRun( lpel_task_t *t );


/** to be called from within a task: */
void LpelTaskExit(  lpel_task_t *ct );
void LpelTaskYield( lpel_task_t *ct );




/******************************************************************************/
/*  STREAM FUNCTIONS                                                          */
/******************************************************************************/

lpel_stream_t *LpelStreamCreate( int);
void LpelStreamDestroy( lpel_stream_t *s);

lpel_stream_desc_t *
LpelStreamOpen( lpel_task_t *t, lpel_stream_t *s, char mode);

void  LpelStreamClose(   lpel_stream_desc_t *sd, int destroy_s);
void  LpelStreamReplace( lpel_stream_desc_t *sd, lpel_stream_t *snew);
void *LpelStreamPeek(    lpel_stream_desc_t *sd);
void *LpelStreamRead(    lpel_stream_desc_t *sd);
void  LpelStreamWrite(   lpel_stream_desc_t *sd, void *item);

lpel_stream_desc_t *LpelStreamPoll(    lpel_streamset_t *set);



void LpelStreamsetPut(  lpel_streamset_t *set, lpel_stream_desc_t *node);
int  LpelStreamsetRemove( lpel_streamset_t *set, lpel_stream_desc_t *node);
int  LpelStreamsetIsEmpty( lpel_streamset_t *set);



lpel_stream_iter_t *LpelStreamIterCreate( lpel_streamset_t *set);
void LpelStreamIterDestroy( lpel_stream_iter_t *iter);

void LpelStreamIterReset(  lpel_stream_iter_t *iter, lpel_streamset_t *set);
int  LpelStreamIterHasNext( lpel_stream_iter_t *iter);
lpel_stream_desc_t *LpelStreamIterNext( lpel_stream_iter_t *iter);
void LpelStreamIterAppend(  lpel_stream_iter_t *iter, lpel_stream_desc_t *node);
void LpelStreamIterRemove(  lpel_stream_iter_t *iter);





#endif /* _LPEL_H_ */
