#include <stdlib.h>
#include <stdio.h>

#include <check.h>
#include "../set.h"


int myints[100];



void printInt(void *pint)
{
  int i = *((int *)pint);
  printf("%d ",i);
  //printf("%p ",pint);
}


void setup(void)
{
  int i;
  // initialize
  for(i=0; i<100; i++) {
    myints[i] = i;
  }
}

void teardown(void)
{
}


START_TEST (test_setadd)
{
  int i;
  set_t s;
  SetAlloc(&s);
  fail_unless( s.size > 0, NULL);
  // add
  for(i=0; i<100; i++) {
    SetAdd(&s, (void *) &(myints[i]) );
  }
  fail_unless( s.cnt == 100, NULL);
  // print
  SetApply(&s, printInt);
  printf("\n");
  SetFree(&s);
}
END_TEST


START_TEST (test_setremove)
{
  int i;
  bool b;
  set_t s;

  SetAlloc(&s);
  for(i=0; i<100; i++)  SetAdd(&s, (void *) &(myints[i]) );

  // remove 10 to 19
  for(i=10; i<=19; i++) {
    b = SetRemove(&s, (void *) &(myints[i]) );
    fail_unless(b == true, NULL);
  }
  fail_unless( s.cnt == 90, NULL);
  SetApply(&s, printInt);
  printf("\n");

  // remove 89 to 80
  for(i=89; i>=80; i--) {
    b = SetRemove(&s, (void *) &(myints[i]) );
    fail_unless(b == true, NULL);
  }
  fail_unless( s.cnt == 80, NULL);
  SetApply(&s, printInt);
  printf("\n");

  // remove 70 to 79
  for(i=70; i<=79; i++) {
    b = SetRemove(&s, (void *) &(myints[i]) );
    fail_unless(b == true, NULL);
  }
  fail_unless( s.cnt == 70, NULL);
  SetApply(&s, printInt);
  printf("\n");

  
  b = SetRemove(&s, (void *) &(myints[75]) );
  fail_unless(b == false, NULL);

  SetFree(&s);
}
END_TEST


Suite *set_suite(void)
{
  Suite *s = suite_create("Set");

  TCase *tc_core = tcase_create("Core");
  tcase_add_unchecked_fixture (tc_core, setup, teardown);
  tcase_add_test(tc_core, test_setadd);
  tcase_add_test(tc_core, test_setremove);
  suite_add_tcase(s, tc_core);

  return s;
}


int main(void)
{
  int number_failed;
  Suite *s = set_suite();
  SRunner *sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  exit( (number_failed==0) ? EXIT_SUCCESS : EXIT_FAILURE  );
}
