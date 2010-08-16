#include <stdlib.h>
#include <stdio.h>

#include <assert.h>
#include <string.h>
#include "../streamset.h"

#define NUM_STREAMS 40

streamset_t *set = NULL;
  
long streams[NUM_STREAMS]; 


typedef struct stream stream_t;

static void header(const char *s);



static void testAdd(void)
{
  int i, add_num = 17;
  streamtbe_t *tbe[2*NUM_STREAMS];
  int idx;

  header("Adding to the streamset");

  /* create the streamset */
  set = StreamsetCreate(2);

  for (i=0; i<add_num; i++) {
    tbe[i] = StreamsetAdd( set, (stream_t *)streams[i], &idx );
    assert( idx == i/STREAMSET_GRP_SIZE );
    StreamsetDebug(set);
  }
  fprintf(stderr, "\n... CLEAN DIRTY ...\n");
  StreamsetPrint(set, NULL);
  StreamsetDebug(set);
   
  /* destroy the streamset */
  StreamsetDestroy(set);
}


static void testEvent(void)
{
  int i, add_num = 6;
  streamtbe_t *tbe[2*NUM_STREAMS];
  int idx;

  header("Events in the streamset");
  
  /* create the streamset */
  set = StreamsetCreate(2);

  for (i=0; i<add_num; i++) {
    tbe[i] = StreamsetAdd( set, (stream_t *)streams[i], &idx );
    assert( idx == i/STREAMSET_GRP_SIZE );
  }
  StreamsetPrint(set, NULL);
  StreamsetDebug(set);
  

  fprintf(stderr, "\nEvents\n");

  StreamsetEvent(set, tbe[2]);
  StreamsetEvent(set, tbe[2]);
  StreamsetEvent(set, tbe[0]);
  StreamsetEvent(set, tbe[1]);
  StreamsetRemove(set, tbe[1]);
  StreamsetReplace(set, tbe[4], (stream_t *)0x201);
  StreamsetDebug(set);
  StreamsetPrint(set, stderr);
  StreamsetDebug(set);


  tbe[add_num] = StreamsetAdd( set, (stream_t *)streams[add_num], &idx );
  assert( idx == 0 );
  StreamsetDebug(set);

  /* destroy the streamset */
  StreamsetDestroy(set);
}



static void testIterate(void)
{
  int i, add_num = 24;
  streamtbe_t *tbe[2*NUM_STREAMS];
  streamtbe_t *it_cur;
  streamtbe_iter_t iter;
  int idx;

  header("Iterator");
  
  /* create the streamset */
  set = StreamsetCreate(2);

  for (i=0; i<add_num; i++) {
    tbe[i] = StreamsetAdd( set, (stream_t *)streams[i], &idx );
    assert( idx == i/STREAMSET_GRP_SIZE );
  }
  StreamsetPrint(set, NULL);
  StreamsetDebug(set);

  /* chain up */
  StreamsetChainStart(set);
  StreamsetChainAdd(set, 0);
  StreamsetChainAdd(set, 1);
  StreamsetChainAdd(set, 3);
  StreamsetChainAdd(set, 5);

  /* iterate */
  fprintf(stderr, "Iterator start:\n");
  StreamsetIterateStart(set, &iter);
  while( StreamsetIterateHasNext(set, &iter) > 0 ) {
    it_cur = StreamsetIterateNext(set, &iter);
    assert( it_cur != NULL );
    fprintf(stderr, "[%p]\n", it_cur->s);
  }

  /* destroy the streamset */
  StreamsetDestroy(set);
}



int main(void)
{
  int i;
  /* init streams */
  for (i=0;i<NUM_STREAMS;i++) {
    streams[i] = 0x100+i;
  }
  
  /* tests */
  //testAdd();
  //testEvent();
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

