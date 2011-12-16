#ifndef _MONITOR_H_
#define _MONITOR_H_
// macros to activate logging or logging mode(s)
#define USE_LOGGING
//#define NO_TASK_EVENT_LOGGING
//#define NO_USER_EVENT_LOGGING

/*
 * since the overhead of monitoring control is too small --> currently not
 * support these options
#ifdef USE_LOGGING
	#ifndef NO_TASK_EVENT_LOGGING
		#define USE_TASK_EVENT_LOGGING
		#ifndef NO_USER_EVENT_LOGGING
			#define USE_USER_EVENT_LOGGING
		#endif
	#endif
#endif
*/

#endif // _MONITOR_H
