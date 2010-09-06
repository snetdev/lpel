#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <assert.h>
#include "../lpel.h"
#include "../stream.h"
#include "../task.h"
#include "../inport.h"
#include "../outport.h"




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

  StreamOpen(self, ch->in, 'r');
  StreamOpen(self, ch->out, 'w');

  while (!term) {
    item = StreamRead( self, ch->in);
    assert( item != NULL );
    //printf("Relay %d: %s", id, item );
    StreamWrite( self, ch->out, item);
    if ( 0 == strcmp( item, "T\n")) {
      term = 1;
      StreamClose(self, ch->in);
      StreamClose(self, ch->out);
      StreamDestroy( ch->out);
      free(ch);
    }
  } // end while
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
  t = TaskCreate( Relay, ch, tattr);
  LpelTaskToWorker(t);

  printf("Created Relay %d\n", depth );
  return (depth > 0) ? PipeElement( out, depth-1) : out;
}



void *Outputter(void *arg)
{
  stream_t *out = (stream_t *)arg;
  outport_t *oport = OutportCreate(out);
  char *item;
  int term = 0;

  while (!term) {
    item = OutportRead(oport);
    assert( item != NULL );
    printf("Out: %s", item );

    if ( 0 == strcmp( item, "T\n")) {
      term = 1;
    }
  } // end while
  
  OutportDestroy(oport);
  return NULL;
}


static void testBasic(void)
{
  stream_t *in, *out;
  char *buf;
  inport_t *iport;
  lpelthread_t *lt;
  lpelconfig_t cfg;

  cfg.num_workers = 2;
  cfg.proc_workers = 2;
  cfg.proc_others = 0;
  cfg.flags = 0;

  LpelInit(&cfg);

  in = StreamCreate();
  out = PipeElement(in, cfg.num_workers*20 - 1);

  lt = LpelThreadCreate(Outputter, out);

  LpelRun();
 
  iport = InportCreate(in);
  do {
    buf = (char *) malloc( 120 * sizeof(char) );
    (void) fgets( buf, 119, stdin  );
    InportWrite(iport, buf);
  } while ( 0 != strcmp(buf, "T\n") );
  InportDestroy(iport);
  
  LpelThreadJoin(lt, NULL);

  LpelCleanup();
}




int main(void)
{
  testBasic();
  printf("test finished\n");
  return 0;
}
