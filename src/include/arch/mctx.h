
#ifndef _MCTX_H_
#define _MCTX_H_

//FIXME
//#define USE_MCTX_PCL
//#define USE_MCTX_SETJMP

#ifdef USE_MCTX_PCL

#include "mctx-pcl.h"


#elif defined(USE_MCTX_SETJMP)

#include "mctx-setjmp.h"

#elif defined(USE_MCTX_X86) || defined(USE_MCTX_X86_64) || defined(USE_MCTX_X86_64_MEM)

#include "mctx-x86.h"

#elif defined(USE_MCTX_UCONTEXT)

#include "mctx-ucontext.h"

#else

#error "No suitable context switching found!"

#endif

#endif /* _MCTX_H_ */
