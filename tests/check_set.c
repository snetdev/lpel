
#include <check.h>
#include "../set.h"


START_TEST (test_setadd)
{
  set_t s;
  SetAlloc(&s);
  fail_unless( s.size > 0, NULL);
}
END_TEST

int main(void)
{
  return 0;
}
