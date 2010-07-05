#include <stdlib.h>
#include <stdio.h>

#include <assert.h>
#include "../debug.h"
#include "../lpel.h"
#include "../task.h"
#include "../stream.h"



stream_t *channel;
int myints[100];



void Consumer(void *arg)
{
  int i;
  void *item;
  DBG("Consumer Task");

  StreamOpen( (stream_t *)arg, 'r');
  for (i=0; i<100; i++) {
    item = StreamRead( (stream_t *)arg );
    fprintf(stderr, "%d ", *((int *)item) );
  }
  TaskExit();
}



void Producer(void *arg)
{
  int i;
  DBG("Producer Task");

  StreamOpen(channel, 'w');
  for (i=0; i<100; i++) {
    StreamWrite(channel, &myints[i] );
  }
  TaskExit();
}



static void testBasic(void)
{
  lpelconfig_t cfg;
  task_t *t;
  int i;

  cfg.num_workers = 2;
  cfg.attr = 0; //LPEL_ATTR_ASSIGNCORE;

  LpelInit(&cfg);

  for (i=0; i<100; i++) myints[i] = i;

  channel = StreamCreate();
  t = TaskCreate(Producer, NULL, 0);
  TaskCreate(Consumer, channel, 0);

  LpelRun();
  
  StreamDestroy(channel);
  LpelCleanup();

}




int main(void)
{
  testBasic();
  return 0;
}
