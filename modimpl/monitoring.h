#ifndef _MONITORING_H_
#define _MONITORING_H_

#include <lpel_common.h>

#define LPEL_MON_TIME   (1<<0)
#define LPEL_MON_TASK		  (1<<1)
#define LPEL_MON_MESSAGE  	  (1<<2)
#define LPEL_MON_WORKER  	  (1<<3)
#define LPEL_MON_STREAM  	  (1<<4)
#define LPEL_MON_MAP  	  (1<<5)
#define LPEL_MON_LOAD	 (1<<6)



struct mon_worker_t;
struct mon_task_t;
struct mon_stream_t;

void LpelMonInit(lpel_monitoring_cb_t *cb, unsigned long flags);
void LpelMonCleanup(void);


struct mon_task_t *LpelMonTaskCreate(unsigned long tid,
    const char *name);


/* special characters */
//#define END_LOG_ENTRY '\n'		// for separate each log entry by one line --> for testing
#define END_LOG_ENTRY 			'#'			// more efficient for file writing, just send when the buffer is full
#define END_STREAM_TRACE 		'|'
#define MESSAGE_TRACE_SEPARATOR ';'			// used to separate message traces
#define WORKER_START_EVENT 		'S'
#define WORKER_WAIT_EVENT 		'W'
#define WORKER_END_EVENT 		'E'

#define LOG_FORMAT_VERSION		"Log format version 2.2 (since 05/03/2012)"


#endif /* _MONITORING_H_ */
