/**
 * THIS FILE MUST NOT BE INCLUDED DIRECTLY
 */

/**
 * Atomic variables and
 * TODO pointer swap and membars
 *
 */

typedef int bool;

typedef struct {
  volatile int counter;
  unsigned char padding[64-sizeof(int)];
} atomic_int;

#define ATOMIC_VAR_INIT(i) { (i) }

/**
 * Initialize atomic variable dynamically
 */
#define atomic_init(v,i)  atomic_store((v),(i))

/**
 * Destroy atomic variable
 */
#define atomic_destroy(v)  /*NOP*/

/**
 * Read atomic variable
 * @param v pointer of type atomic_t
 *
 * Atomically reads the value of @v.
 */
#define atomic_load(v) ((v)->counter)


/**
 * Set atomic variable
 * @param v pointer of type atomic_t
 * @param i required value
 */
#define atomic_store(v,i) (((v)->counter) = (i))


static inline int atomic_fetch_add(atomic_int *v, int i)
{ return __sync_fetch_and_add(&v->counter, i); }

static inline int atomic_fetch_sub(atomic_int *v, int i)
{ return __sync_fetch_and_sub(&v->counter, i); }

static inline int atomic_exchange(atomic_int *v, int value)
{ return __sync_lock_test_and_set(&v->counter, value); }

static inline bool atomic_compare_exchange_strong(atomic_int *v, int *oldval,
                                                  int newval)
{ return __sync_bool_compare_and_swap(&v->counter, *oldval, newval); }
