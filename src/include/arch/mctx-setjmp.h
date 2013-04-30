

#include <setjmp.h>

typedef struct {
  jmp_buf jb;
  void *arg;
} mctx_t;




static inline int mctx_create(mctx_t *mctx, void *func, void *arg, char *sk_addr, long sk_size)
{
  char *stack;
  long p1,p2;

  stack = sk_addr + sk_size - sizeof(long);

  mctx->arg = arg;
  setjmp(mctx->jb);

#ifdef __linux__
#if defined(__x86_64__)
#define JB_PC 7
#define JB_SP 6
#define JB_BP 1
#elif defined(__i386__)
#define JB_PC 5
#define JB_SP 1
#define JB_BP 3
#endif
#endif

#if defined(__FreeBSD__) && defined(__i386__)
  mctx->jb[0]._jb[0] = (int)func;
  mctx->jb[0]._jb[2] = (int)stack;
#elif defined(__FreeBSD__) && defined(__sparc64__)
  mctx->jb[0]._jb[0] = (long)stack;
  mctx->jb[0]._jb[2] = (long)stack;
  mctx->jb[0]._jb[1] = (long)func;
#elif defined(__GLIBC__) && defined(__GLIBC_MINOR__)			\
  && __GLIBC__ >= 2 && __GLIBC_MINOR__ >= 0 && defined(JB_PC) && defined(JB_SP)

  p1 = (long)func;
  p2 = (long)stack;
#if defined(FORTIFY_HACK)
#if defined(__x86_64__)
  __asm__ __volatile__("xorq %%fs:0x30, %0\n\trolq $0x11, %0" : "=r"(p1) : "r"(p1));
  __asm__ __volatile__("xorq %%fs:0x30, %0\n\trolq $0x11, %0" : "=r"(p2) : "r"(p2));
#elif defined(__i386__)
  __asm__ __volatile__("xorl %%gs:0x18, %0\n\troll $9, %0" : "=r"(p1) : "r"(p1));
  __asm__ __volatile__("xorl %%gs:0x18, %0\n\troll $9, %0" : "=r"(p2) : "r"(p2));
#endif
#endif
#if defined(FORTIFY_HACK2)
#if defined(__x86_64__)
  __asm__ __volatile__("xorq %%fs:0x30, %0" : "=r"(p1) : "r"(p1));
  __asm__ __volatile__("xorq %%fs:0x30, %0" : "=r"(p2) : "r"(p2));
#elif defined(__i386__)
  __asm__ __volatile__("xorl %%gs:0x18, %0" : "=r"(p1) : "r"(p1));
  __asm__ __volatile__("xorl %%gs:0x18, %0" : "=r"(p2) : "r"(p2));
#endif
#endif
  mctx->jb[0].__jmpbuf[JB_PC] = p1;
  mctx->jb[0].__jmpbuf[JB_SP] = p2;
  mctx->jb[0].__jmpbuf[JB_BP] = p2;
  mctx->jb[0].__mask_was_saved = 0;

#elif defined(__GLIBC__) && defined(__GLIBC_MINOR__)			\
  && __GLIBC__ >= 2 && __GLIBC_MINOR__ >= 0 && defined(__mc68000__)
  mctx->jb[0].__jmpbuf[0].__aregs[0] = (long) func;
  mctx->jb[0].__jmpbuf[0].__sp = (int *) stack;
#elif defined(__GNU_LIBRARY__) && defined(__i386__)
  mctx->jb[0].__jmpbuf[0].__pc = func;
  mctx->jb[0].__jmpbuf[0].__sp = stack;
#elif defined(__MINGW32__)
  mctx->jb[5] = (long) func;
  mctx->jb[4] = (long) stack;
#elif defined(__APPLE__)
  /* START Apple */
#if defined(__x86_64__)
  *(long *) ((char *) &mctx->jb + 56) = (long) func;
  *(long *) ((char *) &mctx->jb + 16) = (long) stack;
#elif defined(__i386__)
  *(long *) ((char *) &mctx->jb + 48) = (long) func;
  *(long *) ((char *) &mctx->jb + 36) = (long) stack;
#elif defined(__arm__)
  *(long *) ((char *) &mctx->jb + 32) = (long) func;
  *(long *) ((char *) &mctx->jb + 28) = (long) stack;
#else
#error "Unsupported setjmp/longjmp OSX CPU."
#endif
  /* END Apple */
#elif defined(_WIN32) && defined(_MSC_VER)
  /* START Windows */
#if defined(_M_IX86)
  ((_JUMP_BUFFER *) &mctx->jb)->Eip = (long) func;
  ((_JUMP_BUFFER *) &mctx->jb)->Esp = (long) stack;
#elif defined(_M_AMD64)
  ((_JUMP_BUFFER *) &mctx->jb)->Rip = (long) func;
  ((_JUMP_BUFFER *) &mctx->jb)->Rsp = (long) stack;
#else
#error "Unsupported setjmp/longjmp Windows CPU."
#endif
  /* END Windows */
#elif defined(__GLIBC__) && defined(__GLIBC_MINOR__)			\
  && __GLIBC__ >= 2 && __GLIBC_MINOR__ >= 0 && (defined(__powerpc64__) || defined(__powerpc__))
  mctx->jb[0].__jmpbuf[JB_LR] = (int) func;
  mctx->jb[0].__jmpbuf[JB_GPR1] = (int) stack;
#else
#error "Unsupported setjmp/longjmp platform."
#endif

  return 0;
}

static inline void mctx_switch(mctx_t *octx, mctx_t *nctx)
{
  if (setjmp(octx->jb) == 0)
    longjmp(nctx->jb, 1);
}

