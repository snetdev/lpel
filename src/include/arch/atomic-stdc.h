
#include <stdatomic.h>

/* functions we define but that are not in C11 */

typedef char * _Atomic atomic_charptr;
typedef void * _Atomic atomic_voidptr;

#define atomic_destroy(v) ((void)0) /* no-op */

/* NB: our test and set does not modify the expected value */
#define atomic_test_and_set(v, e, d)                    \
    ({                                                  \
        __typeof__(e) tmp = (e);                        \
        atomic_compare_exchange_strong((v), &tmp, (d)); \
    })
