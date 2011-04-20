#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include "../lpel.h"

#define NUM_COLL    (4*10+1)
lpel_stream_t *sinp;
lpel_stream_t *scoll[NUM_COLL];



void *Consumer(void *inarg)
{
  char *msg;
  int i, term;
  lpel_streamset_t lst = NULL;
  lpel_stream_iter_t *iter;

  printf("start Consumer\n" );
  /* open streams */
  for (i=0; i<NUM_COLL; i++) {
    LpelStreamsetPut( &lst, LpelStreamOpen(scoll[i], 'r'));
  }

  iter = LpelStreamIterCreate(&lst);

  term = 0;
  do {
    /* here we do wait */
    printf("Consumer poll\n");
    LpelStreamPoll( &lst);
    printf("Consumer resumes\n");

    printf("Consumer iterates:\n");
    LpelStreamIterReset(iter, &lst);
    while( LpelStreamIterHasNext(iter)) {
      lpel_stream_desc_t *snext = LpelStreamIterNext(iter);
      if ( NULL != LpelStreamPeek( snext )) {
        msg = (char *) LpelStreamRead( snext);
        if (0 == strcmp( msg, "T\n" )) {
          term=1;
        } else {
          printf("%s", msg );
          free(msg);
        }
      }
    }
    //sleep(5);
  } while (0 == term) ;

  /* print & free termination message */
  printf("%s", msg );
  free(msg);

  /* close streams */
  LpelStreamIterReset(iter, &lst);
  while( LpelStreamIterHasNext(iter)) {
    lpel_stream_desc_t *snext = LpelStreamIterNext(iter);
    LpelStreamClose( snext, 1);
  }
  LpelStreamIterDestroy( iter);
  printf("exit Consumer\n" );

  LpelStop();
  return NULL;
}



void *Relay(void *inarg)
{
  void *item;
  char *msg;
  int i, dest;
  lpel_stream_desc_t *out[NUM_COLL];
  lpel_stream_desc_t *in;
  int term = 0;

  printf("start Relay\n" );

  /* open streams */
  for (i=0; i<NUM_COLL; i++) {
    out[i] = LpelStreamOpen(scoll[i], 'w');
  }
  in = LpelStreamOpen(sinp, 'r');

  /* main task: relay to consumer via defined stream */
  while( !term) {
    item = LpelStreamRead(in);
    assert( item != NULL );
    msg = (char *)item;
    if  ( 0 == strcmp( msg, "T\n" ) ) {
      term = 1;
      dest = 0;
    } else {
      dest = atoi(msg);
    }
    if ( 0<dest && dest<NUM_COLL) {
      LpelStreamWrite( out[dest], item);
      printf("Relay dest: %d\n", dest);
    }
  }
  /* terminate msg goes to 0 */
  assert( dest==0 );
  printf("Relay dest: %d\n", dest);

  /* relay to all, close streams */
  for (i=0; i<NUM_COLL; i++) {
    LpelStreamWrite( out[i], item);
    LpelStreamClose( out[i], 0);
  }
  LpelStreamClose(in, 1);
  printf("exit Relay\n" );
  return NULL;
}


static void *Inputter(void *arg)
{
  lpel_stream_desc_t *out = LpelStreamOpen((lpel_stream_t*)arg, 'w');
  char *buf;

  do {
    buf = (char *) malloc( 120 * sizeof(char) );
    (void) fgets( buf, 119, stdin  );
    LpelStreamWrite( out, buf);
  } while ( buf[0] != 'T') ;

  LpelStreamClose( out, 0);
  printf("exit Inputter\n" );
  return NULL;
}


static void testBasic(void)
{
  lpel_config_t cfg;
  int i;
  lpel_task_t *trelay, *tcons, *intask;

  cfg.num_workers = 2;
  cfg.proc_workers = 2;
  cfg.proc_others = 0;
  cfg.flags = 0;
  cfg.node = 0;

  LpelInit(&cfg);

  /* create streams */
  sinp = LpelStreamCreate(0);
  for (i=0; i<NUM_COLL; i++) {
    scoll[i] = LpelStreamCreate(0);
  }

  /* create tasks */
  trelay = LpelTaskCreate( 0, Relay, NULL, 8192);
  LpelTaskRun(trelay);

  tcons = LpelTaskCreate( 1, Consumer, NULL, 8192);
  LpelTaskRun(tcons);


  intask = LpelTaskCreate( -1, Inputter, sinp, 8192);
  LpelTaskRun(intask);

  LpelCleanup();
}




int main(void)
{
  testBasic();
  printf("test finished\n");
  return 0;
}
