#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include "lpel.h"
#include "../modimpl/monitoring.h"




typedef struct {
  lpel_stream_t *in, *out;
  int id;
} channels_t;



void Relay(void *inarg)
{
  channels_t *ch = (channels_t *)inarg;
  int term = 0;
  int id = ch->id;
  char *item;
  lpel_stream_desc_t *in, *out;

  in = LpelStreamOpen(ch->in, 'r');
  out = LpelStreamOpen(ch->out, 'w');

  printf("Relay %d START\n", id);

  while (!term) {
    item = LpelStreamRead( in);
    assert( item != NULL );
    //printf("Relay %d: %s", id, item );
    if ( 0 == strcmp( item, "T\n")) {
      term = 1;
    }
    LpelStreamWrite( out, item);
  } // end while
  LpelStreamClose( in, 1);
  LpelStreamClose( out, 0);
  free(ch);
  printf("Relay %d TERM\n", id);
}


static channels_t *ChannelsCreate(lpel_stream_t *in, lpel_stream_t *out, int id)
{
  channels_t *ch = (channels_t *) malloc( sizeof(channels_t));
  ch->in = in;
  ch->out = out;
  ch->id = id;
  return ch;
}

lpel_stream_t *PipeElement(lpel_stream_t *in, int depth)
{
  lpel_stream_t *out;
  channels_t *ch;
  lpel_task_t *t;
  int wid = depth % 2;
  mon_task_t *mt;

  out = LpelStreamCreate(0);
  ch = ChannelsCreate( in, out, depth);
  t = LpelTaskCreate( wid, Relay, ch, 8192);
  mt = LpelMonTaskCreate(LpelTaskGetId(t), NULL);
  LpelTaskMonitor(t, mt);
  LpelTaskStart(t);

  printf("Created Relay %d\n", depth );
  return (depth > 0) ? PipeElement( out, depth-1) : out;
}



static void Outputter(void *arg)
{
  lpel_stream_desc_t *in = LpelStreamOpen((lpel_stream_t*)arg, 'r'); 
  char *item;
  int term = 0;

  printf("Outputter START\n");

  while (!term) {
    item = LpelStreamRead(in);
    assert( item != NULL );
    printf("Out: %s", item );

    if ( 0 == strcmp( item, "T\n")) {
      term = 1;
    }
    free( item);
  } // end while

  LpelStreamClose( in, 1);
  printf("Outputter TERM\n");

  LpelStop();
}


static void Inputter(void *arg)
{
  lpel_stream_desc_t *out = LpelStreamOpen((lpel_stream_t*)arg, 'w'); 
  char *buf;

  printf("Inputter START\n");
  do {
    buf = fgets( malloc( 120 * sizeof(char) ), 119, stdin  );
    LpelStreamWrite( out, buf);
  } while ( 0 != strcmp(buf, "T\n") );

  LpelStreamClose( out, 0);
  printf("Inputter TERM\n");
}

static void testBasic(void)
{
  lpel_stream_t *in, *out;
  lpel_config_t cfg;
  lpel_task_t *intask, *outtask;
  mon_task_t *mt;

  cfg.num_workers = 2;
  cfg.proc_workers = 2;
  cfg.proc_others = 0;
  cfg.flags = 0;

  unsigned long flags = 1 << 7 - 1;
  LpelMonInit(&cfg.mon, flags);
  LpelInit(&cfg);


  in = LpelStreamCreate(0);
  out = PipeElement(in, cfg.num_workers*20 - 1);

  outtask = LpelTaskCreate( -1, Outputter, out, 8192);
  mt = LpelMonTaskCreate( LpelTaskGetId(outtask), "outtask");
  LpelTaskMonitor(outtask, mt);
  LpelTaskStart(outtask);

  intask = LpelTaskCreate( -1, Inputter, in, 8192);
  mt = LpelMonTaskCreate( LpelTaskGetId(intask), "intask");
  LpelTaskMonitor(intask, mt);
  LpelTaskStart(intask);

  LpelStart(&cfg);
  LpelCleanup();
  LpelMonCleanup();
}




int main(void)
{
  testBasic();
  printf("test finished\n");
  return 0;
}
