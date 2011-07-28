#ifndef _TIMING_H_
#define _TIMING_H_


/*
 * Link with librt:
 *   -lrt
 */
#include <time.h>
#include <unistd.h>

typedef struct timespec timing_t;



#if defined(HAVE_POSIX_TIMERS) && _POSIX_TIMERS > 0

#if defined(_POSIX_CPUTIME)
#  define TIMING_CLOCK  CLOCK_PROCESS_CPUTIME_ID
#elif defined(_POSIX_MONOTONIC_CLOCK)
#  define TIMING_CLOCK  CLOCK_MONOTONIC
#else
#  define TIMING_CLOCK  CLOCK_REALTIME
#endif

/**
 * Current timestamp
 * @param t   pointer to timing_t
 */
#define TIMESTAMP(t) do { \
  (void) clock_gettime(TIMING_CLOCK, (t)); \
} while (0)

#else  /* _POSIX_TIMERS > 0 */


#include <sys/time.h>

/**
 * Current timestamp
 * @param t   pointer to timing_t
 */
#define TIMESTAMP(t) do { \
  struct timeval tv; \
  gettimeofday(&tv, NULL); \
  (t)->tv_sec  = tv.tv_sec; \
  (t)->tv_nsec = tv.tv_usec*1000; \
} while (0)

#endif /* _POSIX_TIMERS > 0 */


#define TIMING_BILLION 1000000000L
#define TIMING_INITIALIZER  {0,0}

/**
 * Start timing
 *
 * Gets current timestamp and stores it in t
 */
static inline void TimingStart(timing_t *t)
{
  TIMESTAMP( t);
}

/**
 * End timing, store the elapsed time in t
 *
 * @pre   TimingStart() was called on t
 */
static inline void TimingEnd(timing_t *t)
{
  timing_t start, end;

  /* get end time  */
  TIMESTAMP( &end);
  /* store start time */
  start = *t;

  /* calculate elapsed time to t,
   * assuming end > start and *.tv_nsec < TIMING_BILLION
   */
  if (end.tv_nsec < start.tv_nsec) {
    start.tv_nsec -= TIMING_BILLION;
    start.tv_sec  += 1L;
  }
  t->tv_nsec = end.tv_nsec - start.tv_nsec;
  t->tv_sec  = end.tv_sec  - start.tv_sec;
}

/**
 * Add a time to another
 *
 * Adds val to t, val is not modified
 */
static inline void TimingAdd(timing_t *t, const timing_t *val)
{
  t->tv_sec  += val->tv_sec;
  t->tv_nsec += val->tv_nsec;
  /* normalize */
  if (t->tv_nsec > TIMING_BILLION) {
    t->tv_nsec -= TIMING_BILLION;
    t->tv_sec  += 1L;
  }
}

static inline void TimingDiff( timing_t *res, const timing_t *start, const timing_t *end)
{
  /* calculate elapsed time to t,
   * assuming end > start and *.tv_nsec < TIMING_BILLION
   */
  *res = *start;
  if (end->tv_nsec < start->tv_nsec) {
    res->tv_nsec -= TIMING_BILLION;
    res->tv_sec  += 1L;
  }
  res->tv_nsec = end->tv_nsec - res->tv_nsec;
  res->tv_sec  = end->tv_sec  - res->tv_sec;
}

/**
 * Set a time
 */
static inline void TimingSet(timing_t *t, const timing_t *val)
{
  t->tv_sec  = val->tv_sec;
  t->tv_nsec = val->tv_nsec;
}

/**
 * Clear a time to 0
 */
static inline void TimingZero(timing_t *t)
{
  t->tv_sec  = 0;
  t->tv_nsec = 0;
}

static inline int TimingEquals(const timing_t *t1, const timing_t *t2)
{
  return ( (t1->tv_sec  == t2->tv_sec) &&
           (t1->tv_nsec == t2->tv_nsec) );
}


/**
 * Get nanoseconds as double type from timing
 */
static inline double TimingToNSec(const timing_t *t)
{
  return ((double)t->tv_sec)*1000000000.0 + (double)t->tv_nsec;
}


/**
 * Get milliseconds as double type from timing
 */
static inline double TimingToMSec(const timing_t *t)
{
  return (((double)t->tv_sec) * 1000.0f) + (((double)t->tv_nsec) / 1000000.0f);
}

/**
 * Calculate the exponential average
 *
 * Updates t by last with weight factor alpha
 * tnew = last*alpha + thist*(1-alpha)
 *
 * Precond: alpha in [0,1]
 */
static inline void TimingExpAvg(timing_t *t,
  const timing_t *last, const float alpha)
{
  double dlast, dhist, dnew;

  dlast = TimingToNSec(last);
  dhist = TimingToNSec(t);

  dnew = dlast*alpha + dhist*(1-alpha);

  t->tv_sec = (unsigned long)(dnew/1000000000.0);
  t->tv_nsec  = (unsigned long)(dnew - (double)t->tv_sec*1000000000);
}



#endif /* _TIMING_H_ */
