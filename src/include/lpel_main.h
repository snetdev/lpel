#ifndef _COMMON_H_
#define _COMMON_H_
#include <lpel_common.h>

struct lpel_stream_desc_t {
  lpel_task_t   *task;        /** the task which opened the stream */
  lpel_stream_t *stream;      /** pointer to the stream */
  char mode;                  /** either 'r' or 'w' */
  struct lpel_stream_desc_t *next; /** for organizing in stream sets */
  struct mon_stream_t *mon;   /** monitoring object */
};



void LpelWorkersInit( int size);
void LpelWorkersCleanup( void);
void LpelWorkersSpawn(void);
void LpelWorkersTerminate(void);


#endif /* _COMMON_H_ */
