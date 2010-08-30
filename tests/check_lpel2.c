#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <assert.h>
#include "../lpel.h"
#include "../stream.h"
#include "../task.h"
#include "../inport.h"

#define NUM_COLL    (4*10+1)
stream_t *sinp;
stream_t *scoll[NUM_COLL];



void Consumer(task_t *self, void *inarg)
{
  void *item;
  char *msg;
  int i, term;

  /* open streams */ 
  for (i=0; i<NUM_COLL; i++) {
    StreamOpen(self, scoll[i], 'r');
  }


  term = 0;
  do {
    stream_t *snext;
    /* here we do wait */
    StreamWaitAny(self);

    printf("Consumer iterates:\n");
    while (StreamIterHasNext(self)) {
      snext = StreamIterNext(self);

      item = StreamPeek( self, snext );
      msg = (char *) item;
      if ( msg != NULL ) {
        printf("%s", msg );
        if (0 == strcmp( msg, "T\n" )) term=1;
      }
    }
    //sleep(5);
  } while (0 == term) ;

  /* close streams */ 
  for (i=0; i<NUM_COLL; i++) {
    StreamClose(self, scoll[i]);
  }
  printf("exit Consumer\n" );
}



void Relay(task_t *self, void *inarg)
{
  void *item;
  char *msg;
  int i, dest;

  /* open streams */ 
  for (i=0; i<NUM_COLL; i++) {
    StreamOpen(self, scoll[i], 'w');
  }
  StreamOpen(self, sinp, 'r');

  /* main task: relay to consumer via defined stream */
  do {
    item = StreamRead(self, sinp);
    assert( item != NULL );
    msg = (char *)item;
    dest = atoi(msg);
    printf("Relay dest: %d\n", dest);
    if ( 0<=dest && dest<NUM_COLL) {
      StreamWrite(self, scoll[dest], item);
    }
  } while ( 0 != strcmp( msg, "T\n" ) );

  /* close streams */ 
  for (i=0; i<NUM_COLL; i++) {
    StreamClose(self, scoll[i]);
  }
  StreamClose(self, sinp);
  printf("exit Relay\n" );
}


/**
 * Read from stdin and feed into a LPEL stream
 */
void *InputReader(void *arg)
{
  char *buf;
  inport_t *in = InportCreate(sinp);

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
  taskattr_t tattr = {0};
  int i;
  task_t *trelay, *tcons;

  cfg.num_workers = 2;
  cfg.proc_workers = 2;
  cfg.proc_others = 2;
  cfg.flags = 0;

  LpelInit(&cfg);

  /* create streams */
  sinp = StreamCreate();
  for (i=0; i<NUM_COLL; i++) {
    scoll[i] = StreamCreate();
  }

  /* create tasks */
  trelay = TaskCreate( Relay, NULL, tattr);
  LpelTaskToWorker(trelay);

  tattr.flags |= TASK_ATTR_WAITANY;
  tcons = TaskCreate( Consumer, NULL, tattr);
  LpelTaskToWorker(tcons);
 
  lt = LpelThreadCreate(InputReader, NULL);

  LpelRun();
  
  /* destroy streams */
  StreamDestroy(sinp);
  for (i=0; i<NUM_COLL; i++) {
    StreamDestroy(scoll[i]);
  }

  LpelCleanup();
  LpelThreadJoin(lt, NULL);
}




int main(void)
{
  testBasic();
  printf("test finished\n");
  return 0;
}
