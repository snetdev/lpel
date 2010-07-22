#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <assert.h>
#include "../timing.h"





static void test1(void)
{
  timing_t last, hist;

  TimingZero(&hist);
  TimingStart(&last);
  sleep(1);
  TimingEnd(&last);
  printf("last: %f\n", TimingToMSec(&last));
  TimingExpAvg(&hist, &last, 0.1f);

  printf("EAVG: %f\n", TimingToMSec(&hist));
}


int main(int argc)
{

  test1();

  return 0;
}
