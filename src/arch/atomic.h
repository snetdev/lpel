#ifndef _ATOMIC_H_
#define _ATOMIC_H_

/*  
 * Defined here:
 * atomic_int - c11
 * atomic_uint - c11
 * atomic_long - c11
 * atomic_ulong - c11
 * atomic_llong - c11
 * atomic_ullong - c11
 * atomic_charptr - custom
 * atomic_voidptr - custom
 *
 * ATOMIC_VAR_INIT - c11
 * atomic_init - c11
 * atomic_destroy - custom: deallocate/deinit
 * atomic_load - c11
 * atomic_store - c11
 * atomic_exchange - c11
 * atomic_test_and_set - custom: does not modify expected value if comparison fails
 * atomic_fetch_add - c11
 * atomic_fetch_sub - c11
 */

#if (__STDC_VERSION >= 199901L)
#include <stdbool.h>
#else
typedef int bool;
#endif


#if (__STDC_VERSION__ >= 201112L) && !defined(__STDC_NO_ATOMICS__)
#include "atomic-stdc.h"

#elif  (__GNUC__ > 4) || \
  (__GNUC__==4) && (__GNUC_MINOR__ >= 1) && (__GNUC_PATCHLEVEL__ >= 0)
/* gcc/icc compiler builtin atomics  */
#include "atomic-builtin.h"

#else
#warning "Cannot determine which atomics to use, fallback to pthread locked atomics!"
#include "atomic-pthread.h"

#endif


#endif /* _ATOMIC_H_ */
