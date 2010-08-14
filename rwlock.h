#ifndef _RWLOCK_H_
#define _RWLOCK_H_


#include <assert.h>
#include "sysdep.h"

#define SWAP xchg

/**
 * A Readers/Writer Lock, for a single writer and NUM_READER readers
 *
 * using local spinning
 */

#define intxCacheline (64/sizeof(int))

#if 0
typedef struct {
  volatile int l;
  int padding[intxCacheline-1];
} spinlock_t[1];

static inline void spinlock_init(spinlock_t v)
{
  v[0].l = 0;
}

static inline void spinlock_lock(spinlock_t v)
{
  while (SWAP( (int *)&v[0].l, 1) != 0) {
    /* spin locally (only reads) - reduces bus traffic */
    while (v[0].l);
  }
}

static inline void spinlock_unlock(spinlock_t v)
{
  WMB();
  v[0].l = 0;
}
#endif

typedef struct {
  /*TODO
  volatile int l;
  int padding[intxCacheline-1];
  */
} rwlock_t[1];

static inline void rwlock_init( rwlock_t v, int num_readers )
{
  //TODO
}

static inline void rwlock_reader_lock( rwlock_t v, int ridx )
{
  while(1) {
    WMB();
    while( L.writer != 0 ); /*spin*/
    
    // set me as trying
    L.readers[ridx] = 1;

    WMB();
    if (L.writer == 0) {
      // no writer: success!
      break;
    }

    /* backoff to let writer go through */
    L.readers[ridx] = 0;
  }
}

static inline void rwlock_reader_unlock( rwlock_t v, int ridx )
{
  WMB();
  L.readers[ridx] = 0;
}


static inline void rwlock_writer_lock( rwlock_t v )
{
  /* assume no competing writer and a sane lock/unlock usage */
  assert( L.writer == 0 );

  L.writer = 1;
  /*
   * now write lock is held, but we have to wait until current
   * readers have finished
   */
  WMB();
  for (int i=0; i<N; i++) {
    while( L.readers[i] != 0 ); /*spin*/
  }
}

static inline void rwlock_writer_unlock( rwlock_t v )
{
  WMB();
  L.writer = 0;
}

#endif /* _RWLOCK_H_ */
