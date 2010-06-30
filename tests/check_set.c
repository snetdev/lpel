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
  fail_unless( s.size == SET_INITSIZE, NULL);
  // add up to fill completely
  for(i=0; i<SET_INITSIZE; i++) {
    SetAdd(&s, (void *) &(myints[i]) );
  }
  fail_unless( s.cnt  == SET_INITSIZE &&
               s.size == SET_INITSIZE,
               NULL);
  // add one more
  SetAdd(&s, (void *) &(myints[SET_INITSIZE]) );
  fail_unless( s.cnt  == SET_INITSIZE+1 &&
               s.size == SET_INITSIZE+SET_DELTASIZE,
               NULL);
  // again up to fill completely
  for(i=SET_INITSIZE+1; i<SET_INITSIZE+SET_DELTASIZE; i++) {
    SetAdd(&s, (void *) &(myints[i]) );
  }
  fail_unless( s.cnt  == SET_INITSIZE+SET_DELTASIZE &&
               s.size == SET_INITSIZE+SET_DELTASIZE,
               NULL);

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
  void *e1, *e2;

  SetAlloc(&s);
  for(i=0; i<SET_INITSIZE+3*SET_DELTASIZE+1; i++) {
    SetAdd(&s, (void *) &(myints[i]) );
  }
  fail_unless( s.cnt  == SET_INITSIZE+3*SET_DELTASIZE+1 &&
               s.size == SET_INITSIZE+4*SET_DELTASIZE,
               NULL);
  
  // last element
  e1 = s.array[s.cnt-1];
  // element to remove
  i = SET_INITSIZE+1;
  e2 = s.array[i];

  b = SetRemove(&s, e2);
  fail_unless( b && (s.array[i] == e1), NULL );
  // removing again same element
  b = SetRemove(&s, e2);
  fail_if( b, "removing twice the same succedded" );

  // remove SET_DELTASIZE elements
  for(i=0; i<SET_DELTASIZE; i++) {
    b = SetRemove(&s, (void *) &(myints[i]) );
    fail_unless(b == true, NULL);
  }
  fail_unless( s.cnt ==  SET_INITSIZE+2*SET_DELTASIZE &&
               s.size == SET_INITSIZE+4*SET_DELTASIZE,
               NULL);

  // removing one more should trigger reallocating to a smaller size
  // remove the last
  e1 = s.array[s.cnt-1];
  b = SetRemove(&s, e1);
  fail_unless( b &&
               s.cnt ==  SET_INITSIZE+2*SET_DELTASIZE-1 &&
               s.size == SET_INITSIZE+3*SET_DELTASIZE,
               NULL);
  b = SetRemove(&s, e1);
  fail_if( b, "removing twice the same succedded" );

  // print
  SetApply(&s, printInt);
  printf("\n");

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
