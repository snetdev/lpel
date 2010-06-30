#include <stdlib.h>
#include <stdio.h>

#include <check.h>
#include "../cpuassign.h"



void setup(void)
{
}

void teardown(void)
{
}


START_TEST (test_cpuassign_query)
{
  int n;
  n = CpuAssignQueryNumCpus();
  fail_unless( n>0, "At least one CPU is always availible!");
}
END_TEST

START_TEST (test_cpuassign_exclusively)
{
  bool b;
  b = CpuAssignCanExclusively();
  fail_if( b, "Not root!");
}
END_TEST

Suite *cpuassign_suite(void)
{
  Suite *s = suite_create("CpuAssign");

  TCase *tc_core = tcase_create("Core");
  tcase_add_unchecked_fixture (tc_core, setup, teardown);
  tcase_add_test(tc_core, test_cpuassign_query);
  tcase_add_test(tc_core, test_cpuassign_exclusively);
  suite_add_tcase(s, tc_core);

  return s;
}


int main(void)
{
  int number_failed;
  Suite *s = cpuassign_suite();
  SRunner *sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  exit( (number_failed==0) ? EXIT_SUCCESS : EXIT_FAILURE  );
}
