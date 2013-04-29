/**
 * THIS FILE MUST NOT BE INCLUDED DIRECTLY
 */

#include <pthread.h>

#ifdef HAVE_PTHREAD_SPIN_INIT
#define ATOMIC_PTHREAD_USE_SPINLOCK
#endif

#ifdef ATOMIC_PTHREAD_USE_SPINLOCK
#  define ATOMIC_VAR_INIT(i) { (i), 1 }
#  define __lock_type pthread_spinlock_t
#  define __lock_init(v) (void) pthread_spin_init( &(v)->lock, PTHREAD_PROCESS_PRIVATE)
#  define __lock_destroy(v) (void) pthread_spin_destroy( &(v)->lock)
#  define __lock_get(v) (void) pthread_spin_lock( &(v)->lock)
#  define __lock_release(v) (void) pthread_spin_unlock( &(v)->lock)
#else
#  define ATOMIC_VAR_INIT(i) { (i), PTHREAD_MUTEX_INITIALIZER }
#  define __lock_type pthread_mutex_t
#  define __lock_init(v) (void) pthread_mutex_init( &(v)->lock, NULL)
#  define __lock_destroy(v) (void) pthread_mutex_destroy( &(v)->lock)
#  define __lock_get(v) (void) pthread_mutex_lock( &(v)->lock)
#  define __lock_release(v) (void) pthread_mutex_unlock( &(v)->lock)
#endif
    
#define __do_typedef(T, name) \
typedef struct { T val; __lock_type lock; } atomic_##name

__do_typedef(int, int);
__do_typedef(unsigned int, uint);
__do_typedef(long, long);
__do_typedef(unsigned long, ulong);
__do_typedef(long long, llong);
__do_typedef(unsigned long long, ullong);
__do_typedef(char*, charptr);
__do_typedef(void*, voidptr);
#undef __do_typedef

#define atomic_init(V, I)                                \
    do {                                                 \
        __lock_init(V);                                  \
        (V)->val = (I);                                  \
    } while(0)

#define atomic_destroy(V) do { __lock_destroy(V); } while(0)

#define atomic_load(V)                                                  \
    ({                                                                  \
        __typeof__((V)->val)  tmp;                                       \
        __lock_get(V);                                                  \
        tmp = (V)->val;                                                  \
        __lock_release(V);                                              \
        tmp;                                                            \
    })

#define atomic_store(V, I)                               \
    do {                                                 \
        __lock_get(V);                                   \
        (V)->val = (I);                                  \
        __lock_release(V);                               \
    } while(0)

#define atomic_exchange(V, I)                                           \
    ({                                                                  \
    __typeof__((V)->val) tmp;                                            \
    __lock_get(V);                                                      \
    tmp = (V)->val;                                                     \
    (V)->val = (I);                                                     \
    __lock_release(V);                                                  \
    tmp;                                                                \
    })

#define __atomic_fetch_X(V, I, Op)                                      \
    ({                                                                  \
    __typeof__((V)->val) tmp;                                           \
    __lock_get(V);                                                      \
    tmp = (V)->val;                                                     \
    (V)->val = (V)->val Op (I);                                         \
    __lock_release(V);                                                  \
    tmp;                                                                \
    })

#define atomic_fetch_add(V, I) __atomic_fetch_X(V, I, +) 
#define atomic_fetch_sub(V, I) __atomic_fetch_X(V, I, -) 

#define atomic_test_and_set(V, E, D)                                    \
            ({                                                          \
                bool cmpres;                                            \
                __lock_get(V);                                          \
                if (cmpres = ((V)->val == (E))) (V)->val = (D);         \
                __lock_release(V);                                      \
                cmpres;                                                 \
            })
