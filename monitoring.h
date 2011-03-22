#ifndef _MONITORING_H_
#define _MONITORING_H_



#define LPEL_MON_TASK_TIMES   (1<<0)
#define LPEL_MON_TASK_STREAMS (1<<1)


/**
 * monitoring context
 */
typedef struct monctx_t monctx_t;


/**
 * monitoring information on a task
 */
typedef struct mon_task_t mon_task_t;


/**
 * monitoring information on a stream
 */
typedef struct mon_stream_t mon_stream_t;


monctx_t *LpelMonContextCreate(char *name);
void LpelMonContextDestroy( monctx_t *mon);

mon_task_t *LpelMonTaskCreate(unsigned long tid,
    char *name, unsigned long flags);

void LpelMonTaskDestroy(mon_task_t *);

void LpelMonTaskAssign(mon_task_t *mt, monctx_t *ctx);

char *LpelMonTaskGetName(mon_task_t *mt);

void LpelMonDebug( monctx_t *mon, const char *fmt, ...);


/*****************************************************************************
 * CALLBACK FUNCTIONS
 ****************************************************************************/

enum taskstate_t;


void LpelMonTaskStart(mon_task_t *mt);
void LpelMonTaskStop(mon_task_t *mt, enum taskstate_t state);


mon_stream_t *LpelMonStreamOpen(mon_task_t *mt, unsigned int sid, char mode);
void LpelMonStreamClose(mon_stream_t *ms);
void LpelMonStreamReplace(mon_stream_t *ms, unsigned int new_sid);

void LpelMonStreamMoved(mon_stream_t *ms, void *item);
void LpelMonStreamBlockon(mon_stream_t *ms);
void LpelMonStreamWakeup(mon_stream_t *ms);


#endif /* _MONITORING_H_ */
