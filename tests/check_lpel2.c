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
  stream_mh_t *in[NUM_COLL];

  printf("start Consumer\n" );
  /* open streams */ 
  for (i=0; i<NUM_COLL; i++) {
    in[i] = StreamOpen(self, scoll[i], 'r');
  }


  term = 0;
  do {
    stream_mh_t *snext;
    /* here we do wait */
    printf("Consumer waits\n");
    TaskWaitOnAny(self);
    printf("Consumer resumes\n");

    printf("Consumer iterates:\n");
    for (i=0; i<NUM_COLL; i++) {
      snext = in[i];

      item = StreamPeek( snext );
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
    StreamClose(in[i], true);
  }
  printf("exit Consumer\n" );
}



void Relay(task_t *self, void *inarg)
{
  void *item;
  char *msg;
  int i, dest;
  stream_mh_t *out[NUM_COLL];
  stream_mh_t *in;
  int term = 0;

  printf("start Relay\n" );

  /* open streams */ 
  for (i=0; i<NUM_COLL; i++) {
    out[i] = StreamOpen(self, scoll[i], 'w');
  }
  in = StreamOpen(self, sinp, 'r');

  /* main task: relay to consumer via defined stream */
  while( !term) {
    item = StreamRead(in);
    assert( item != NULL );
    msg = (char *)item;
    if  ( 0 == strcmp( msg, "T\n" ) ) {
      term = 1;
      dest = 0;
    } else {
      dest = atoi(msg);
    }
    if ( 0<dest && dest<NUM_COLL) {
      StreamWrite( out[dest], item);
      printf("Relay dest: %d\n", dest);
    }
  }
  /* terminate msg goes to 0 */
  assert( dest==0 );
  printf("Relay dest: %d\n", dest);
  StreamWrite( out[dest], item);

  /* close streams */ 
  for (i=0; i<NUM_COLL; i++) {
    StreamClose(out[i], false);
  }
  StreamClose(in, true);
  printf("exit Relay\n" );
}



static void testBasic(void)
{
  lpelconfig_t cfg;
  taskattr_t tattr = {0};
  int i;
  task_t *trelay, *tcons;

  cfg.num_workers = 2;
  cfg.proc_workers = 2;
  cfg.proc_others = 0;
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

  //tattr.flags |= TASK_ATTR_WAITANY;
  tcons = TaskCreate( Consumer, NULL, tattr);
  LpelTaskToWorker(tcons);
 

  LpelRun();
  {
    char *buf;
    inport_t *in = InportCreate(sinp);
    do {
      buf = (char *) malloc( 120 * sizeof(char) );
      (void) fgets( buf, 119, stdin  );
      InportWrite(in, buf);
      printf("Input: %s\n", buf );
    } while ( 0 != strcmp(buf, "T\n") );
    printf("end reading input\n" );
    InportDestroy(in);
  }
  
  LpelCleanup();

}




int main(void)
{
  testBasic();
  printf("test finished\n");
  return 0;
}
