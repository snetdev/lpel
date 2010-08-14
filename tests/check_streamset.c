#include <stdlib.h>
#include <stdio.h>

#include <assert.h>
#include "../streamset.h"


streamset_t *set = NULL;

typedef struct stream stream_t;

static void testAdd(void)
{
  int i, num = 5;
  void *streams[] = { 0x100, 0x101, 0x102, 0x103, 0x104 }; 
  streamtbe_t *tbe[num*2];
  int idx;

  set = StreamsetCreate(2);

  for (i=0; i<num; i++) {
    tbe[i] = StreamsetAdd( set, (stream_t *)streams[i], &idx );
    fprintf(stderr, "%d ", idx);
  }
  fprintf(stderr, "\n");
  StreamsetDebug(set);
  StreamsetPrint(set, NULL);
  
  
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

  StreamsetDestroy(set);
}


int main(void)
{
  testAdd();

  return 0;
}
