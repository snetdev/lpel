
#ifndef _MCTX_H_
#define _MCTX_H_

//FIXME
//#define USE_MCTX_PCL

#ifdef USE_MCTX_PCL


#include "mctx-pcl.h"


#elif defined(__amd64__) || defined(__i386__)

#include "mctx-x86.h"

#elif defined(__linux__)

#include "mctx-ucontext.h"

#else

#include "mctx-setjmp.h"

#endif

#endif /* _MCTX_H_ */
