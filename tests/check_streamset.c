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
    printf("%d ", idx);
  }
  printf("\n");
  StreamsetPrint(set, stdout);
  
  StreamsetPrint(set, stdout);
  
  StreamsetEvent(set, tbe[2]);
  StreamsetEvent(set, tbe[2]);
  StreamsetEvent(set, tbe[0]);
  StreamsetPrint(set, stdout);
 
  StreamsetRemove(set, tbe[1]);
  StreamsetPrint(set, stdout);
  
  tbe[5] = StreamsetAdd( set, (stream_t *)0x105, &idx );
  printf("%d ", idx);
  StreamsetPrint(set, stdout);

  StreamsetDestroy(set);
}


int main(void)
{
  testAdd();

  return 0;
}
