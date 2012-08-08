/**
 * THIS FILE MUST NOT BE INCLUDED DIRECTLY
 */

#define __do_typedef(T, name) \
typedef struct { volatile T val; char padding[64-sizeof(T)]; } atomic_##name

__do_typedef(int, int);
__do_typedef(unsigned int, uint);
__do_typedef(long, long);
__do_typedef(unsigned long, ulong);
__do_typedef(long long, llong);
__do_typedef(unsigned long long, ullong);
__do_typedef(char*, charptr);
__do_typedef(void*, voidptr);
#undef __do_typedef

#define ATOMIC_VAR_INIT(i) { (i) }

#define atomic_init(v,i)  atomic_store(v, i)

#define atomic_destroy(v) ((void)0) /*NOP*/

#define atomic_load(v) ((v)->val)

#define atomic_store(v,i) do { (v)->val = (i); } while(0)

#define atomic_exchange(v, i) __sync_lock_test_and_set(&(v)->val, (i))

#define atomic_test_and_set(v, e, d)  __sync_bool_compare_and_swap(&(v)->val, (e), (d))

#define atomic_fetch_add(v, i) __sync_fetch_and_add(&(v)->val, (i))
#define atomic_fetch_sub(v, i) __sync_fetch_and_sub(&(v)->val, (i))

