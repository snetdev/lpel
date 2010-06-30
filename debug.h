#ifndef _DEBUG_H_
#define _DEBUG_H_


#ifndef NODEBUG
#  include <stdio.h>
#  define DBG(fmt,...) do {\
        fprintf(stderr, "DBG[%s:%d] ", __FILE__, __LINE__);\
        fprintf(stderr, fmt, ## __VA_ARGS__ );\
        fprintf(stderr, "\n");\
      } while (0)
#else
#  define DBG(fmt,...) /*nop*/
#endif


#endif /* _DEBUG_H_ */
