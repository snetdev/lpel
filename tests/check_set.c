#include <stdlib.h>
#include <stdio.h>

#include <assert.h>
#include "../set.h"

int myints[100];


void printInt(void *pint)
{
  int i = *((int *)pint);
  printf("%d ",i);
  //printf("%p ",pint);
}


static void testAdd(void)
{
  int i;
  set_t s;
  SetAlloc(&s);
  assert( s.size == SET_INITSIZE );
  // add up to fill completely
  for(i=0; i<SET_INITSIZE; i++) {
    SetAdd(&s, (void *) &(myints[i]) );
  }
  assert( s.cnt==SET_INITSIZE &&  s.size==SET_INITSIZE );
  
  // add one more
  SetAdd(&s, (void *) &(myints[SET_INITSIZE]) );
  assert( s.cnt  == SET_INITSIZE+1 &&
          s.size == SET_INITSIZE+SET_DELTASIZE );
  
  // again up to fill completely
  for(i=SET_INITSIZE+1; i<SET_INITSIZE+SET_DELTASIZE; i++) {
    SetAdd(&s, (void *) &(myints[i]) );
  }
  assert( s.cnt  == SET_INITSIZE+SET_DELTASIZE &&
          s.size == SET_INITSIZE+SET_DELTASIZE    );

  // print
  SetApply(&s, printInt);
  printf("\n");

  SetFree(&s);
}


static void testRemove(void)
{
  int i;
  bool b;
  set_t s;
  void *e1, *e2;

  SetAlloc(&s);
  for(i=0; i<SET_INITSIZE+3*SET_DELTASIZE+1; i++) {
    SetAdd(&s, (void *) &(myints[i]) );
  }
  assert( s.cnt  == SET_INITSIZE+3*SET_DELTASIZE+1 &&
          s.size == SET_INITSIZE+4*SET_DELTASIZE      );
  
  // last element
  e1 = s.array[s.cnt-1];
  // element to remove
  i = SET_INITSIZE+1;
  e2 = s.array[i];

  b = SetRemove(&s, e2);
  assert( b && (s.array[i] == e1) );
  // removing again same element
  b = SetRemove(&s, e2);
  assert( !b );

  // remove SET_DELTASIZE elements
  for(i=0; i<SET_DELTASIZE; i++) {
    b = SetRemove(&s, (void *) &(myints[i]) );
    assert( b );
  }
  assert( s.cnt ==  SET_INITSIZE+2*SET_DELTASIZE &&
          s.size == SET_INITSIZE+4*SET_DELTASIZE    );

  // removing one more should trigger reallocating to a smaller size
  // remove the last
  e1 = s.array[s.cnt-1];
  b = SetRemove(&s, e1);
  assert( b &&
          s.cnt ==  SET_INITSIZE+2*SET_DELTASIZE-1 &&
          s.size == SET_INITSIZE+3*SET_DELTASIZE
          );
  b = SetRemove(&s, e1);
  assert( !b );

  // print
  SetApply(&s, printInt);
  printf("\n");

  SetFree(&s);
}


int main(void)
{
  int i;
  // initialize
  for(i=0; i<100; i++) {
    myints[i] = i;
  }
  testAdd();
  testRemove();

  return 0;
}
