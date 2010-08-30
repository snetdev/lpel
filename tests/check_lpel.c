#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <assert.h>
#include "../lpel.h"
#include "../task.h"
#include "../inport.h"






void Consumer(task_t *self, void *inarg)
{
  void *item;
  char *msg;

  StreamOpen( self, (stream_t *)inarg, 'r');
  do {
    item = StreamRead( self, (stream_t *)inarg );
    msg = (char *) item;
    printf("read Consumer %s\n", msg );
  } while ( 0 != strcmp( msg, "T\n" ) );
  StreamClose( self, (stream_t *)inarg);
  StreamDestroy( (stream_t *)inarg);
  printf("exit Consumer\n" );
}



void Relay(task_t *self, void *inarg)
{
  void *item;
  char *msg;
  stream_t *to, *from;
  to = StreamCreate();
  from = (stream_t *)inarg;
  taskattr_t tattr = {0};
  task_t *tcons;

  StreamOpen(self, to, 'w');
  StreamOpen(self, from, 'r');
  tcons = TaskCreate(Consumer, (void *)to, tattr);
  LpelTaskToWorker(tcons);
  do {
    item = StreamRead(self, from);
    assert( item != NULL );
    msg = (char *)item;
    printf("read Relay %s\n", msg );
    StreamWrite(self, to, item);
  } while ( 0 != strcmp( msg, "T\n" ) );
  printf("exit Relay\n" );
  StreamClose(self, from);
  StreamDestroy(from);
  sleep(1);
  StreamClose(self, to);
}


/**
 * Read from stdin and feed into a LPEL stream
 */
void *InputReader(void *arg)
{
  char *buf;
  stream_t *instream = StreamCreate();
  inport_t *in = InportCreate(instream);
  taskattr_t tattr = {0};
  task_t *t;

  t = TaskCreate( Relay, (void *)instream, tattr);
  LpelTaskToWorker(t);

  do {
    buf = (char *) malloc( 120 * sizeof(char) );
    (void) fgets( buf, 119, stdin  );
    InportWrite(in, buf);
  } while ( 0 != strcmp(buf, "T\n") );
  //printf("exit InputReader\n" );

  InportDestroy(in);
  return NULL;
}

static void testBasic(void)
{
  lpelconfig_t cfg;
  lpelthread_t *lt;

  cfg.num_workers = 2;
  cfg.proc_workers = 2;
  cfg.proc_others = 0;
  cfg.flags = 0;

  LpelInit(&cfg);

  lt = LpelThreadCreate(InputReader, NULL);

  LpelRun();
  
  LpelCleanup();
  LpelThreadJoin(lt, NULL);
}




int main(void)
{
  testBasic();
  printf("test finished\n");
  return 0;
}
