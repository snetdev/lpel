#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <error.h>
#include <pthread.h>


#include "pthr_streams.h"

#include "arch/timing.h"

#define RING_SIZE 100
#define STACK_SIZE  (16*1024) /* 16k */

#define ROUNDS 10000

static int ids[RING_SIZE];
static pthr_stream_t *streams[RING_SIZE];

static pthread_t master;

typedef struct {
  int round;
  unsigned int hopcnt;
  int term;
} msg_t;


static void PrintEOR(msg_t *msg) {
  /* print end of round message for process 0 */
  //printf("Finished round %d, hops %d\n", msg->round, msg->hopcnt);
}


void *Process(void *arg)
{
  int id = *(int *)arg;
  pthr_stream_t *in, *out;
  msg_t *msg;
  int term = 0;
  timing_t ts;

  out = streams[id];


  if (id==0) {
    in = streams[RING_SIZE-1];
    
    printf("Sending message, ringsize %d, rounds %d\n", RING_SIZE, ROUNDS);
    /* send the first message */
    msg = malloc( sizeof *msg);
    msg->round = 1;
    msg->term = 0;
    msg->hopcnt = 0;

  
    TimingStart( &ts);
    PthrStreamWrite( out, msg);
  } else {
    in = streams[id-1];
  }


  while(!term) {
    msg = PthrStreamRead( in);
    msg->hopcnt++;

    if (id==0) {
      PrintEOR(msg);
      /* next round */
      msg->round++;
      if (msg->round == ROUNDS) msg->term = 1;
    }
    if (msg->term) term = 1;

    PthrStreamWrite( out, msg);
  }

  /* read the msg a last time, free it */
  if (id==0) {
    msg = PthrStreamRead( in);
    TimingEnd( &ts);
    msg->hopcnt++;
    PrintEOR(msg);
    printf("Time to pass the message %u times: %.2f ms\n", msg->hopcnt, TimingToMSec( &ts));
    free(msg);
  }

  PthrStreamDestroy( in);

  pthread_exit(NULL);
}



static void CreateTask(int id)
{
  int res;
  pthread_t p;
  pthread_attr_t attr;

  (void) pthread_attr_init( &attr);

  res = pthread_attr_setstacksize(&attr, STACK_SIZE);
  if (res != 0) {
    error(1, res, "Cannot set stack size!");
  }

  if (id != 0) {
    res = pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
    if (res != 0) {
      error(1, res, "Cannot set detached!");
    }
  }

  res = pthread_create( (id==0)?&master:&p,
      &attr, Process, &ids[id]);
  if (res != 0) {
    error(1, res, "Cannot create pthread!");
  }
  pthread_attr_destroy( &attr);
}


static void CreateRing(void)
{
  int i;
  for (i=0; i<RING_SIZE; i++) {
    ids[i] = i;
    streams[i] = PthrStreamCreate();
  }

  for (i=RING_SIZE-1; i>=0; i--) {
    CreateTask(i);
  }
}


static void testBasic(void)
{
  CreateRing();
}




int main(void)
{
  testBasic();
  pthread_join(master, NULL);
  printf("test finished\n");
  return 0;
}
