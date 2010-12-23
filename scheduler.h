#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_


typedef struct schedctx schedctx_t;

struct task;

void SchedInit( int num_workers);
void SchedCleanup( void);
schedctx_t *SchedCreate( int wid);
void SchedDestroy( schedctx_t *sc);
void SchedMakeReady( schedctx_t* sc, struct task *t);
struct task *SchedFetchReady( schedctx_t *sc);



#endif /* _SCHEDULER_H_ */
