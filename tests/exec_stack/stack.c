
#include <stdlib.h>
#include <stdio.h>

#include <malloc.h>

#include "arch/mctx.h"


static mctx_t ctx_main, ctx1, ctx2;

#define STACKSIZE 4096

void func1(void *arg)
{
  printf("Function 1, arg: %p\n", arg);
  mctx_switch( &ctx1, &ctx2 );
  printf("Function 2 again, arg: %p\n", arg);
  mctx_switch( &ctx1, &ctx2 );
}

void func2(void *arg)
{
  printf("Function 2, arg: %p\n", arg);
  mctx_switch( &ctx2, &ctx1 );
  printf("Function 2 again, arg: %p\n", arg);
  mctx_switch( &ctx2, &ctx_main );
}





int main(int argc, char **argv)
{
  void *st1 = valloc(STACKSIZE);
  void *st2 = valloc(STACKSIZE);
 
  mctx_create( &ctx1, func1, (void*)0x100, st1, STACKSIZE);
  mctx_create( &ctx2, func2, (void*)0x200, st2, STACKSIZE);


  printf("Function main calling func1\n");
  mctx_switch( &ctx_main, &ctx1 );
  printf("Function main again\n");


  printf("Test finished\n");
  return 0;
}

