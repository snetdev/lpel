
#ifndef _MCTX_H_
#define _MCTX_H_

#ifdef __amd64__

#include "mctx-amd64.h"

#elif defined(__i386__)

#include "mctx-amd64.h"

#elif defined(__linux__)

#include "mctx-ucontext.h"

#else

#include "mctx-setjmp.h"

#endif 

#endif /* _MCTX_H_ */
