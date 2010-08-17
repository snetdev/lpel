#include <stdlib.h>
#include <stdio.h>

#include <assert.h>
#include "../streamtable.h"


streamtable_t tab = NULL;

typedef struct stream stream_t;

static void testAdd(void)
{
  unsigned long *cnt;

  cnt = StreamtablePut( &tab, (stream_t *)0x01 , 'w' );
  *cnt = 101;
  StreamtablePrint(&tab, stdout);
  
  cnt = StreamtablePut( &tab, (stream_t *)0x02 , 'w' );
  *cnt = 102;
  StreamtablePrint(&tab, stdout);

  cnt = StreamtablePut( &tab, (stream_t *)0x03 , 'r' );
  *cnt = 103;
  StreamtablePrint(&tab, stdout);

  cnt = StreamtablePut( &tab, (stream_t *)0x04 , 'r' );
  *cnt = 104;
  StreamtablePrint(&tab, stdout);
}


static void testRemove(void)
{
  unsigned int rem;

  StreamtableMark( &tab, (stream_t *)0x02 );
  StreamtablePrint(&tab, stdout);
  
  StreamtableMark( &tab, (stream_t *)0x04 );
  StreamtablePrint(&tab, stdout);
  
  rem = StreamtableClean( &tab );
  StreamtablePrint(&tab, stdout);
  printf("(%u removed)\n", rem);

  StreamtableMark( &tab, (stream_t *)0x01 );
  StreamtablePrint(&tab, stdout);
  
  /*
  rem = StreamtableClean( &tab );
  StreamtablePrint(&tab, stdout);
  printf("(%u removed)\n", rem);
  */

  StreamtableMark( &tab, (stream_t *)0x03 );
  StreamtablePrint(&tab, stdout);
  
  rem = StreamtableClean( &tab );
  StreamtablePrint(&tab, stdout);
  printf("(%u removed)\n", rem);

}


int main(void)
{
  testAdd();
  testRemove();

  return 0;
}
