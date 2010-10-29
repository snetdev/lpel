#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include "../lpel.h"
#include "../scheduler.h"
#include "../stream.h"
#include "../task.h"




typedef struct {
  stream_t *in, *out;
  int id;
} channels_t;



void Relay(task_t *self, void *inarg)
{
  channels_t *ch = (channels_t *)inarg;
  int term = 0;
  int id = ch->id;
  char *item;
  stream_desc_t *in, *out;

  in = StreamOpen( self, ch->in, 'r');
  out = StreamOpen( self, ch->out, 'w');
  
  printf("Relay %d START\n", id);

  while (!term) {
    item = StreamRead( in);
    assert( item != NULL );
    //printf("Relay %d: %s", id, item );
    if ( 0 == strcmp( item, "T\n")) {
      term = 1;
    }
    StreamWrite( out, item);
  } // end while
  StreamClose( in, true);
  StreamClose( out, false);
  free(ch);
  printf("Relay %d TERM\n", id);
}


static channels_t *ChannelsCreate(stream_t *in, stream_t *out, int id)
{
  channels_t *ch = (channels_t *) malloc( sizeof(channels_t));
  ch->in = in;
  ch->out = out;
  ch->id = id;
  return ch;
}

stream_t *PipeElement(stream_t *in, int depth)
{
  stream_t *out;
  channels_t *ch;
  taskattr_t tattr = {0};
  task_t *t;

  out = StreamCreate();
  ch = ChannelsCreate( in, out, depth);
  t = TaskCreate( Relay, ch, &tattr);
  SchedAssignTask( t, t->uid % 2);

  printf("Created Relay %d\n", depth );
  return (depth > 0) ? PipeElement( out, depth-1) : out;
}



static void Outputter(task_t *self, void *arg)
{
  stream_desc_t *in= StreamOpen( self, (stream_t*)arg, 'r'); 
  char *item;
  int term = 0;

  printf("Outputter START\n");

  while (!term) {
    item = StreamRead(in);
    assert( item != NULL );
    printf("Out: %s", item );

    if ( 0 == strcmp( item, "T\n")) {
      term = 1;
    }
    free( item);
  } // end while

  StreamClose( in, true);
  
  printf("Outputter TERM\n");
  //SchedTerminate();
}


static void Inputter(task_t *self, void *arg)
{
  stream_desc_t *out = StreamOpen( self, (stream_t*)arg, 'w'); 
  char *buf;

  printf("Inputter START\n");
  do {
    buf = (char *) malloc( 120 * sizeof(char) );
    (void) fgets( buf, 119, stdin  );
    StreamWrite( out, buf);
  } while ( 0 != strcmp(buf, "T\n") );

  StreamClose( out, false);
  printf("Inputter TERM\n");
}

static void testBasic(void)
{
  stream_t *in, *out;
  lpelthread_t *inlt, *outlt;
  lpelconfig_t cfg;
  task_t *intask, *outtask;
  taskattr_t tattr = { 0,0};

  cfg.num_workers = 2;
  cfg.proc_workers = 2;
  cfg.proc_others = 0;
  cfg.flags = 0;
  cfg.node = -1;

  LpelInit(&cfg);

  in = StreamCreate();
  out = PipeElement(in, cfg.num_workers*20 - 1);

  outtask = TaskCreate( Outputter, out, &tattr);
  outlt = LpelThreadCreate( SchedWrapper, outtask, false, "outputter");

  intask = TaskCreate( Inputter, in, &tattr);
  inlt = LpelThreadCreate( SchedWrapper, intask, false, "inputter");

  LpelThreadJoin( inlt);
  LpelThreadJoin( outlt);
      
  printf("Calling SchedTerminate()\n");
  SchedTerminate();

  LpelCleanup();
}




int main(void)
{
  testBasic();
  printf("test finished\n");
  return 0;
}
