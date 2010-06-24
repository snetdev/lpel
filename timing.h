#ifndef _TIMING_H_
#define _TIMING_H_

#include <time.h>

typedef struct timespec timing_t;

extern void TimingStart(timing_t *t);
extern void TimingEnd(timing_t *t);
extern void TimingAdd(timing_t *t, const timing_t *val);
extern void TimingSet(timing_t *t, const timing_t *val);
extern void TimingZero(timing_t *t);
extern void TimingExpAvg(timing_t *t, const timing_t *last, const float alpha);



#endif /* _TIMING_H_ */
