#ifndef _SPINLOCK_H_
#define _SPINLOCK_H_

/**
 * Simple spinlock implementation
 *
 * Test-and-test-and-set style spinlocks
 * using local spinning
 *
 *
 * @TODO: Alternatively one could use pthread_spin_lock/-unlock
 *       operations. Could be switched by defines.
 * @TODO: Alternative 2: extrude atomics
 */



#define intxCacheline (64/sizeof(int))

typedef struct {
  volatile int l;
  int padding[intxCacheline-1];
} spinlock_t;

static inline void SpinlockInit(spinlock_t *v)
{
  /*v->l = 0; with fence */
  __sync_lock_release(&v->l);
}

static inline void SpinlockCleanup(spinlock_t *v)
{ /*NOP*/ }

static inline void SpinlockLock(spinlock_t *v)
{
  while ( __sync_lock_test_and_set( &v->l, 1) != 0) {
    /* spin locally (only reads) - reduces bus traffic */
    while (v->l);
  }
}

static inline void SpinlockUnlock(spinlock_t *v)
{
  /* flush store buffers */
  __sync_lock_release(&v->l);
}

#endif /* _SPINLOCK_H_ */
