
#ifndef _MCTX_H_
#define _MCTX_H_

#if defined(__amd64__) || defined(__i386__)

#include "mctx-x86.h"

#elif defined(__linux__)

#include "mctx-ucontext.h"

#else

#include "mctx-setjmp.h"

#endif 

#endif /* _MCTX_H_ */
