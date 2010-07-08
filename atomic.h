#ifndef _ATOMIC_H_
#define _ATOMIC_H_

/*
 * NOTE:
 *
 * This implementation relies on the GCC 4.1.2+ Atomic builtins.
 *
 * If you cannot link because of undefined references to the
 * __sync_* functions, try compiling with -march=i486.
 * i386 is not supported.
 */


typedef struct {
  volatile int counter;
} atomic_t;

#define ATOMIC_INIT(i) { (i) }

/**
 * Read atomic variable
 * @param v pointer of type atomic_t
 *
 * Atomically reads the value of @v.
 */
#define atomic_read(v) ((v)->counter)


/**
 * Set atomic variable
 * @param v pointer of type atomic_t
 * @param i required value
 */
#define atomic_set(v,i) (((v)->counter) = (i))


/**
 * Increment atomic variable
 * @param v pointer of type atomic_t
 *
 * Atomically increments @v by 1.
 */
static inline void atomic_inc( atomic_t *v )
{
  (void)__sync_fetch_and_add(&v->counter, 1);
}

/**
 * Decrement atomic variable
 * @param v: pointer of type atomic_t
 *
 * Atomically decrements @v by 1
 */
static inline void atomic_dec( atomic_t *v )
{
  (void)__sync_fetch_and_sub(&v->counter, 1);
}


/**
 * Atomic fetch and increment
 * @param v: pointer of type atomic_t
 * @return the value before incrementing
 */
static inline int fetch_and_inc( atomic_t *v )
{
  return __sync_fetch_and_add(&v->counter, 1);
}


/**
 * Atomic fetch and decrement
 * @param v: pointer of type atomic_t
 * @return the value before decrementing
 */
static inline int fetch_and_dec( atomic_t *v )
{
  return __sync_fetch_and_sub(&v->counter, 1);
}


typedef atomic_t sequencer_t;
#define SEQUENCER_INIT  ATOMIC_INIT(0)

static inline int ticket( sequencer_t *s ) {
  return __sync_fetch_and_add(&s->counter, 1);
}

#endif /* _ATOMIC_H_ */
