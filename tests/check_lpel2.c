#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include <assert.h>
#include "../debug.h"
#include "../cpuassign.h"
#include "../lpel.h"
#include "../stream.h"
#include "../task.h"
#include "../inport.h"

#define NUM_COLL    (4*10+1)
stream_t *sinp;
stream_t *scoll[NUM_COLL];



void Consumer(task_t *t, void *inarg)
{
  void *item;
  char *msg;
  int i, term;

  /* open streams */ 
  for (i=0; i<NUM_COLL; i++) {
    StreamOpen(t, scoll[i], 'r');
  }


  term = 0;
  do {
    stream_t *snext;
    /* here we do wait */
    StreamWaitAny(t);

    printf("Consumer iterates:\n");
    while (StreamIterHasNext(t)) {
      snext = StreamIterNext(t);

      item = StreamPeek( t, snext );
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
    StreamClose(t, scoll[i]);
  }
  printf("exit Consumer\n" );
}



void Relay(task_t *t, void *inarg)
{
  void *item;
  char *msg;
  int i, dest;

  /* open streams */ 
  for (i=0; i<NUM_COLL; i++) {
    StreamOpen(t, scoll[i], 'w');
  }
  StreamOpen(t, sinp, 'r');

  /* main task: relay to consumer via defined stream */
  do {
    item = StreamRead(t, sinp);
    assert( item != NULL );
    msg = (char *)item;
    dest = atoi(msg);
    printf("Relay dest: %d\n", dest);
    if ( 0<=dest && dest<NUM_COLL) {
      StreamWrite(t, scoll[dest], item);
    }
  } while ( 0 != strcmp( msg, "T\n" ) );

  /* close streams */ 
  for (i=0; i<NUM_COLL; i++) {
    StreamClose(t, scoll[i]);
  }
  StreamClose(t, sinp);
  printf("exit Relay\n" );
}


/**
 * Read from stdin and feed into a LPEL stream
 */
void *InputReader(void *arg)
{
  char *buf;
  inport_t *in = InportCreate(sinp);

  CpuAssignToCore(1);

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
  pthread_t th;
  int i;

  cfg.num_workers = 2;
  cfg.flags = LPEL_ATTR_ASSIGNCORE;

  LpelInit(&cfg);

  /* create streams */
  sinp = StreamCreate();
  for (i=0; i<NUM_COLL; i++) {
    scoll[i] = StreamCreate();
  }

  /* create tasks */
  TaskCreate( Relay, NULL, 0);
  TaskCreate( Consumer, NULL, TASK_ATTR_WAITANY );
  pthread_create(&th, NULL, InputReader, NULL);

  LpelRun();
  
  /* destroy streams */
  StreamDestroy(sinp);
  for (i=0; i<NUM_COLL; i++) {
    StreamDestroy(scoll[i]);
  }

  LpelCleanup();
  pthread_join(th, NULL);
}




int main(void)
{
  testBasic();
  printf("test finished\n");
  return 0;
}
