#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include "../lpel.h"
#include "../scheduler.h"
#include "../stream.h"
#include "../task.h"

#define NUM_COLL    (4*10+1)
stream_t *sinp;
stream_t *scoll[NUM_COLL];



void Consumer(task_t *self, void *inarg)
{
  char *msg;
  int i, term;
  stream_list_t lst;
  stream_iter_t *iter;

  printf("start Consumer\n" );
  /* open streams */ 
  for (i=0; i<NUM_COLL; i++) {
    StreamListAppend( &lst, StreamOpen(self, scoll[i], 'r'));
  }

  iter = StreamIterCreate(&lst);

  term = 0;
  do {
    /* here we do wait */
    printf("Consumer poll\n");
    StreamPoll( &lst);
    printf("Consumer resumes\n");

    printf("Consumer iterates:\n");
    StreamIterReset(&lst, iter);
    while( StreamIterHasNext(iter)) {
      stream_desc_t *snext = StreamIterNext(iter);
      if ( NULL != StreamPeek( snext )) {
        msg = (char *) StreamRead( snext);
        printf("%s", msg );
        if (0 == strcmp( msg, "T\n" )) term=1;
      }
    }
    //sleep(5);
  } while (0 == term) ;

  /* close streams */ 
  StreamIterReset(&lst, iter);
  while( StreamIterHasNext(iter)) {
    stream_desc_t *snext = StreamIterNext(iter);
    StreamClose( snext, true);
  }
  StreamIterDestroy( iter);
  printf("exit Consumer\n" );
  
  SchedTerminate();
}



void Relay(task_t *self, void *inarg)
{
  void *item;
  char *msg;
  int i, dest;
  stream_desc_t *out[NUM_COLL];
  stream_desc_t *in;
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


static void Inputter(task_t *self, void *arg)
{
  stream_desc_t *out = StreamOpen( self, (stream_t*)arg, 'w'); 
  char *buf;

  do {
    buf = (char *) malloc( 120 * sizeof(char) );
    (void) fgets( buf, 119, stdin  );
    StreamWrite( out, buf);
  } while ( 0 != strcmp(buf, "T\n") );

  StreamClose( out, false);
}


static void testBasic(void)
{
  lpelconfig_t cfg;
  taskattr_t tattr = {0};
  int i;
  task_t *trelay, *tcons, *intask;
  lpelthread_t *inlt;

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
  SchedAssignTask( trelay, trelay->uid % 2);

  //tattr.flags |= TASK_ATTR_WAITANY;
  tcons = TaskCreate( Consumer, NULL, tattr);
  SchedAssignTask( tcons, tcons->uid % 2);
 
  
  intask = TaskCreate( Inputter, sinp, tattr);
  inlt = LpelThreadCreate( SchedWrapper, intask, false, "inputter");

  LpelThreadJoin( inlt);
  
  LpelCleanup();
}




int main(void)
{
  testBasic();
  printf("test finished\n");
  return 0;
}
