#ifndef _DEBUG_H_
#define _DEBUG_H_


#ifndef NODEBUG
#  include <stdio.h>
#  define DBG(fmt,...) do {\
        fprintf(stderr, "DBG[%s:%d] " fmt "\n", \
                __FILE__, __LINE__,  ## __VA_ARGS__ );\
      } while (0)
#else
#  define DBG(fmt,...) /*nop*/
#endif


#endif /* _DEBUG_H_ */
