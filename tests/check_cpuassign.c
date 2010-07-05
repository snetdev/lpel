#include <stdlib.h>
#include <stdio.h>

#include <assert.h>
#include "../cpuassign.h"




static void testQuery(void)
{
  int n;
  n = CpuAssignQueryNumCpus();
  printf("CPUs: %d\n", n);
  assert( n>0 );
}

static void testExclusively(int is_root)
{
  bool b;
  b = CpuAssignCanExclusively();
  
  if (is_root==0) assert(!b);
  if (is_root!=0) assert( b);
}


int main(int argc)
{
  printf("argc=%d\n", argc);

  testQuery();
  testExclusively(argc>1);

  return 0;
}
