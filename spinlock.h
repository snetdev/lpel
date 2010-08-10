#ifndef _SPINLOCK_H_
#define _SPINLOCK_H_


#include "sysdep.h"

#define SWAP xchg

/**
 * Simple spinlocks
 *
 * Test-and-test-and-set style spinlocks
 * using local spinning
 */

typedef volatile int spinlock_t[1];

static inline void spinlock_init(spinlock_t v)
{
  v[0] = 0;
}

static inline void spinlock_lock(spinlock_t v)
{
  while (SWAP((int *)v, 1) != 0) {
    /* spin locally (only reads) - reduces bus traffic */
    while (v[0]);
  }
}

static inline void spinlock_unlock(spinlock_t v)
{
  WMB();
  v[0] = 0;
}

#endif /* _SPINLOCK_H_ */
