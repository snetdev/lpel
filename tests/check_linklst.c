#include <stdlib.h>
#include <stdio.h>

#include <assert.h>
#include <string.h>
#include "../linklst.h"


list_hnd_t list;

#define NUM 0x10
list_node_t *pool[NUM];

static void header(const char *s);


static void testAppend(void)
{
  int i;
  header("Appending");
  list = NULL;
  for (i=0; i<10; i++) {
    ListAppend( &list, pool[i]);
  }
  ListPrint( &list, stdout );
}

static void testIter(int num)
{
  int i;
  list_iter_t *iter;
  header("Iterate");

  for (i=0; i<num; i++) {
    ListAppend( &list, pool[i]);
  }
  ListPrint( &list, stdout );

  iter = ListIterCreate( &list);
  while( ListIterHasNext(iter) ) {
    list_node_t *node = ListIterNext(iter);
    printf("%lX ", (long) ListNodeGet(node));
  }
  printf("\n");
}


static void testIterAppend(int sizeinit)
{
  int i;
  list = NULL;
  list_iter_t *iter;
  
  header("Iter Appending");
  for (i=0; i<sizeinit; i++) {
    ListAppend( &list, pool[i]);
  }
  ListPrint( &list, stdout );

  iter= ListIterCreate( &list);
  while( ListIterHasNext(iter) ) {
    list_node_t *node = ListIterNext(iter);
    printf("%lx ", (long) ListNodeGet(node));
    if (i<NUM) {
      printf("Append i=%x while iter\n", i);
      ListAppend( &list, pool[i] );
      i++;
    }
  }
  printf("\n");
  ListPrint( &list, stdout );

}

void testIterRemoveAllFromBeg(void)
{
  int i;
  list_iter_t *iter;
  
  header("Iter Removing all from beginning");
  list = NULL;
  for (i=0; i<NUM; i++) {
    ListAppend( &list, pool[i]);
  }
  ListPrint( &list, stdout );

  iter= ListIterCreate( &list);
  while( ListIterHasNext(iter) ) {
    list_node_t *node = ListIterNext(iter);
    printf("%lx ", (long) ListNodeGet(node));
    ListIterRemove(iter);
  }
  printf("\n");
  ListPrint( &list, stdout );
  
}

static void testIterRemoveLast(void)
{
  int i;
  list_iter_t *iter;
  
  header("Iter Removing last");
  list = NULL;
  for (i=0; i<NUM; i++) {
    ListAppend( &list, pool[i]);
  }
  ListPrint( &list, stdout );

  iter= ListIterCreate( &list);
  while( ListIterHasNext(iter) ) {
    list_node_t *node = ListIterNext(iter);
    printf("%lx ", (long) ListNodeGet(node));
    if (!ListIterHasNext(iter)) ListIterRemove(iter);
  }
  printf("\n");
  ListPrint( &list, stdout );
}

static void testIterRemoveRange(void)
{
  int i;
  list_iter_t *iter;
  
  header("Iter Removing range [0x107 to 0x10a]");
  list = NULL;
  for (i=0; i<NUM; i++) {
    ListAppend( &list, pool[i]);
  }
  ListPrint( &list, stdout );

  iter= ListIterCreate( &list);
  while( ListIterHasNext(iter) ) {
    long val;
    list_node_t *node = ListIterNext(iter);
    val = (long) ListNodeGet(node);
    if (val >= 0x107 && val <= 0x10a) {
      ListIterRemove(iter);
      printf("%lx ", (long) ListNodeGet(node));
    }
  }
  printf("\n");
  ListPrint( &list, stdout );
}

int main(void)
{
  long i;
  /* init streams */
  for (i=0; i<NUM; i++) {
    pool[i] = ListNodeCreate( (void *) (0x100+i) );
  }
  
  /* tests */
  testAppend();
  testIter(5);
  testIter(1);
  testIterAppend(1);
  testIterAppend(10);
  testIterRemoveAllFromBeg();
  testIterRemoveLast();
  testIterRemoveRange();
  return 0;
}


/*** HELPER *****************************************************************/

static void header(const char *s)
{
  int i,len = strlen(s);
  fprintf(stderr,"=== %s ", s);
  for (i=0; i<(79-len-5); i++) fprintf(stderr,"=");
  fprintf(stderr,"\n");
}

