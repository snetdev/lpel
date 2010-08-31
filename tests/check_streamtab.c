#include <stdlib.h>
#include <stdio.h>

#include <assert.h>
#include <string.h>
#include "../streamtab.h"

#define NUM_STREAMS 40

streamtab_t tab;
  
long streams[NUM_STREAMS]; 


typedef struct stream stream_t;

static void header(const char *s);



static void testAdd(void)
{
  int i, add_num = 17;
  streamtbe_t *tbe[2*NUM_STREAMS];
  int idx;

  header("Adding to the streamtab");

  /* create the streamtab */
  StreamtabInit(&tab, 2);

  for (i=0; i<add_num; i++) {
    tbe[i] = StreamtabAdd( &tab, (stream_t *)streams[i], &idx );
    assert( idx == i/STREAMTAB_GRP_SIZE );
    StreamtabDebug(&tab);
  }
  fprintf(stderr, "\n... CLEAN DIRTY ...\n");
  StreamtabPrint(&tab, NULL);
  StreamtabDebug(&tab);
   
  /* destroy the streamtab */
  StreamtabCleanup(&tab);
}


static void testEvent(void)
{
  int i, add_num = 6;
  streamtbe_t *tbe[2*NUM_STREAMS];
  int idx;

  header("Events in the streamtab");
  
  /* create the streamtab */
  StreamtabInit(&tab, 2);

  for (i=0; i<add_num; i++) {
    tbe[i] = StreamtabAdd( &tab, (stream_t *)streams[i], &idx );
    assert( idx == i/STREAMTAB_GRP_SIZE );
  }
  StreamtabPrint(&tab, NULL);
  StreamtabDebug(&tab);
  

  fprintf(stderr, "\nEvents\n");

  StreamtabEvent(&tab, tbe[2]);
  StreamtabEvent(&tab, tbe[2]);
  StreamtabEvent(&tab, tbe[0]);
  StreamtabEvent(&tab, tbe[1]);
  StreamtabRemove(&tab, tbe[1]);
  StreamtabReplace(&tab, tbe[4], (stream_t *)0x201);
  StreamtabDebug(&tab);
  StreamtabPrint(&tab, stderr);
  StreamtabDebug(&tab);


  tbe[add_num] = StreamtabAdd( &tab, (stream_t *)streams[add_num], &idx );
  assert( idx == 0 );
  StreamtabDebug(&tab);

  /* destroy the streamtab */
  StreamtabCleanup(&tab);
}



static void testIterate(void)
{
  int i, add_num = 24;
  streamtbe_t *tbe[2*NUM_STREAMS];
  streamtbe_t *it_cur;
  streamtbe_iter_t iter;
  int idx;

  header("Iterator");
  
  /* create the streamtab */
  StreamtabInit(&tab, 2);

  for (i=0; i<add_num; i++) {
    tbe[i] = StreamtabAdd( &tab, (stream_t *)streams[i], &idx );
    assert( idx == i/STREAMTAB_GRP_SIZE );
  }
  StreamtabPrint(&tab, NULL);
  StreamtabDebug(&tab);

  /* chain up */
  StreamtabChainStart(&tab);
  StreamtabChainAdd(&tab, 0);
  StreamtabChainAdd(&tab, 1);
  StreamtabChainAdd(&tab, 3);
  StreamtabChainAdd(&tab, 5);

  /* iterate */
  fprintf(stderr, "Iterator start:\n");
  StreamtabIterateStart(&tab, &iter);
  while( StreamtabIterateHasNext(&tab, &iter) > 0 ) {
    it_cur = StreamtabIterateNext(&tab, &iter);
    assert( it_cur != NULL );
    fprintf(stderr, "[%p]\n", it_cur->s);
  }

  /* destroy the streamtab */
  StreamtabCleanup(&tab);
}



int main(void)
{
  int i;
  /* init streams */
  for (i=0;i<NUM_STREAMS;i++) {
    streams[i] = 0x100+i;
  }
  
  /* tests */
  testAdd();
  testEvent();
  testIterate();

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

