#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lpel.h"
#include "arch/timing.h"

#define RING_SIZE 100
#define STACK_SIZE (16*1024) /* 16k */

#define ROUNDS 10000

static int ids[RING_SIZE];
static lpel_stream_t *streams[RING_SIZE];


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
  lpel_stream_desc_t *in, *out;
  msg_t *msg;
  int term = 0;
  timing_t ts;

  out = LpelStreamOpen(streams[id], 'w');

  if (id==0) {
    in = LpelStreamOpen(streams[RING_SIZE-1], 'r');

    printf("Sending message, ringsize %d, rounds %d\n", RING_SIZE, ROUNDS);
    /* send the first message */
    msg = malloc( sizeof *msg);
    msg->round = 1;
    msg->term = 0;
    msg->hopcnt = 0;

    TimingStart( &ts);
    LpelStreamWrite( out, msg);
  } else {
    in = LpelStreamOpen(streams[id-1], 'r');
  }


  while(!term) {
    msg = LpelStreamRead( in);
    msg->hopcnt++;

    if (id==0) {
      PrintEOR(msg);
      /* next round */
      msg->round++;
      if (msg->round == ROUNDS) msg->term = 1;
    }
    if (msg->term) term = 1;

    LpelStreamWrite( out, msg);
  }

  /* read the msg a last time, free it */
  if (id==0) {
    msg = LpelStreamRead( in);
    TimingEnd( &ts);
    msg->hopcnt++;
    PrintEOR(msg);
    printf("Time to pass the message %u times: %.2f ms\n", msg->hopcnt, TimingToMSec( &ts));
    free(msg);
  }

  LpelStreamClose( in, 1);
  LpelStreamClose( out, 0);

  return NULL;
}



static void CreateTask(int id)
{
  lpel_task_t *t;
  int i;

  i = 0;
  // i = id % 2;
  // i = (id < RING_SIZE/2) ? 0 : 1;

  t = LpelTaskCreate( i, Process, &ids[id], STACK_SIZE);
  LpelTaskRun( t );
}


static void CreateRing(void)
{
  int i;
  timing_t ts;

  for (i=0; i<RING_SIZE; i++) {
    ids[i] = i;
    streams[i] = LpelStreamCreate(0);
  }

  TimingStart( &ts );
  for (i=RING_SIZE-1; i>=0; i--) {
    CreateTask(i);
  }
  TimingEnd( &ts) ;
  printf("Time to create %d tasks: %.2f ms\n", RING_SIZE, TimingToMSec( &ts));
}


static void testBasic(void)
{
  lpel_config_t cfg;
  /*
  cfg.num_workers = 2;
  cfg.proc_workers = 2;
  */
  cfg.num_workers = 1;
  cfg.proc_workers = 1;

  cfg.proc_others = 0;
  cfg.flags = 0;
  cfg.node = 0;

  LpelInit(&cfg);

  CreateRing();

  LpelStop();
  LpelCleanup();
}




int main(void)
{
  testBasic();
  printf("test finished\n");
  return 0;
}

