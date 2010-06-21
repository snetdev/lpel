
#ifndef _LPEL_H_
#define _LPEL_H_

#include "bool.h"

/*
 * General LPEL management
 */

extern void LpelInit(const int nworkers);
extern void LpelRun(void);
extern void LpelCleanup(void);

/*
 * Stream management
 */

typedef struct stream stream_t;

extern stream_t *StreamCreate(void);
extern void StreamDestroy(stream_t *s);
extern bool StreamOpen(stream_t *s, char mode);
extern void *StreamPeek(stream_t *s);
extern void *StreamRead(stream_t *s);
extern bool StreamIsSpace(stream_t *s);
extern void StreamWrite(stream_t *s, void *item);


/*
 * Task management
 */

typedef struct task task_t;

typedef enum {
  TASK_TYPE_NORMAL,
  TASK_TYPE_IO
} tasktype_t;



#endif /* _LPEL_H_ */
