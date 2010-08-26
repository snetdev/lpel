#ifndef _SPINLOCK_H_
#define _SPINLOCK_H_

/**
 * Simple spinlock implementation
 *
 * Test-and-test-and-set style spinlocks
 * using local spinning
 *
 * Assumes xchg has load-with-aquire semantics
 * (holds for the current implementations)
 *
 * TODO: Alternatively one could use pthread_spin_lock/-unlock
 *       operations. Could be switched by defines.
 *
 */

/** following file implements xchg and WMB */
#include "sysdep.h"


#define intxCacheline (64/sizeof(int))

typedef struct {
  volatile int l;
  int padding[intxCacheline-1];
} spinlock_t;

static inline void SpinlockInit(spinlock_t *v)
{
  v->l = 0;
}

static inline void SpinlockCleanup(spinlock_t *v)
{ /*NOP*/ }

static inline void SpinlockLock(spinlock_t *v)
{
  while ( xchg( (int *)&v->l, 1) != 0) {
    /* spin locally (only reads) - reduces bus traffic */
    while (v->l);
  }
}

static inline void SpinlockUnlock(spinlock_t *v)
{
  /* flush store buffers */
  WMB();
  v->l = 0;
}

#endif /* _SPINLOCK_H_ */
