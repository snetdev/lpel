

#include <setjmp.h>

typedef jmp_buf mctx_t;




static inline int mctx_create(mctx_t *mctx, void *func, void *arg, char *sk_addr, long sk_size)
{
  unsigned int x,y;
  unsigned long z;

  if (getcontext(mctx))
    return -1;

  mctx->uc_link = NULL;

  mctx->uc_stack.ss_sp = sk_addr;
  mctx->uc_stack.ss_size = sk_size - 2*sizeof(long);
  mctx->uc_stack.ss_flags = 0;

  /*
   * From libtask, by Ross Cox:
   *
   * All this magic is because you have to pass makecontext a
   * function that takes some number of word-sized variables,
   * and on 64-bit machines pointers are bigger than words.
   */
  z = (unsigned long) arg;
  y = z;
  z >>= 16;
  x = z >>16;
  makecontext(mctx, func, 2, y, x);

  return 0;
}

static inline void mctx_switch(mctx_t *octx, mctx_t *nctx)
{
  (void) swapcontext(octx, nctx);
}

