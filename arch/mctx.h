
#ifndef _MCTX_H_
#define _MCTX_H_

#include <ucontext.h>

typedef ucontext_t mctx_t;


static inline int mctx_create(mctx_t *mctx, void *func, void *arg, char *sk_addr, long sk_size)
{
  if (getcontext(mctx))
    return -1;

  mctx->uc_link = NULL;

  mctx->uc_stack.ss_sp = sk_addr;
  mctx->uc_stack.ss_size = sk_size - 2*sizeof(long);
  mctx->uc_stack.ss_flags = 0;

  makecontext(mctx, func, 2, arg);

  return 0;
}

static inline void mctx_switch(mctx_t *octx, mctx_t *nctx)
{
  (void) swapcontext(octx, nctx);
}


#endif /* _MCTX_H_ */
