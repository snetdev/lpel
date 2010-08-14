#include <stdlib.h>
#include <stdio.h>

#include <assert.h>
#include "../streamset.h"

#define NUM_STREAMS 20

streamset_t *set = NULL;
  
long streams[NUM_STREAMS]; 


typedef struct stream stream_t;

static void testAdd(void)
{
  int i, add_num = 17;
  streamtbe_t *tbe[2*NUM_STREAMS];
  int idx;


  for (i=0; i<add_num; i++) {
    tbe[i] = StreamsetAdd( set, (stream_t *)streams[i], &idx );
    fprintf(stderr, "%d ", idx);
    StreamsetDebug(set);
  }
  fprintf(stderr, "\n... CLEAN DIRTY ...\n");
  StreamsetPrint(set, NULL);
  StreamsetDebug(set);
   
  /*
  StreamsetEvent(set, tbe[2]);
  StreamsetEvent(set, tbe[2]);
  StreamsetEvent(set, tbe[0]);
  StreamsetDebug(set);
  StreamsetPrint(set, NULL);
 
  StreamsetRemove(set, tbe[1]);
  StreamsetDebug(set);
  StreamsetPrint(set, NULL);
  
  tbe[5] = StreamsetAdd( set, (stream_t *)0x105, &idx );
  fprintf(stderr, "%d ", idx);
  StreamsetDebug(set);
  StreamsetPrint(set, NULL);
  */

}


int main(void)
{
  int i;
  /* init streams */
  for (i=0;i<NUM_STREAMS;i++) {
    streams[i] = 0x100+i;
  }
  /* create the streamset */
  set = StreamsetCreate(2);
  
  /* tests */
  testAdd();

  /* destroy the streamset */
  StreamsetDestroy(set);

  return 0;
}
