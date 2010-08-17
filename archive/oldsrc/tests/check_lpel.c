#include <stdlib.h>
#include <stdio.h>

#include <assert.h>
#include "../debug.h"
#include "../lpel.h"
#include "../task.h"
#include "../stream.h"

#define NUM_ITEMS 100000

stream_t *channel;
int myints[NUM_ITEMS];



void Consumer(task_t *t, void *inarg)
{
  int i;
  void *item;
  DBG("Consumer Task");

  StreamOpen( t, (stream_t *)inarg, 'r');
  for (i=0; i<NUM_ITEMS; i++) {
    item = StreamRead( t, (stream_t *)inarg );
    //fprintf(stderr, "%d ", *((int *)item) );
  }
  StreamDestroy(channel);
}



void Producer(task_t *t, void *inarg)
{
  int i;
  DBG("Producer Task");

  StreamOpen(t, channel, 'w');
  for (i=0; i<NUM_ITEMS; i++) {
    StreamWrite(t, channel, &myints[i] );
  }
}



static void testBasic(void)
{
  lpelconfig_t cfg;
  task_t *t;
  int i;

  cfg.num_workers = 2;
  cfg.flags = LPEL_ATTR_ASSIGNCORE;

  LpelInit(&cfg);

  for (i=0; i<NUM_ITEMS; i++) myints[i] = i;

  channel = StreamCreate();
  t = TaskCreate(Producer, NULL, 0);
  TaskCreate(Consumer, channel, 0);

  LpelRun();
  
  LpelCleanup();

}




int main(void)
{
  testBasic();
  return 0;
}
