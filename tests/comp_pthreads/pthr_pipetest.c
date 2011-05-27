#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include <error.h>
#include <pthread.h>

#include "pthr_streams.h"


#include "arch/timing.h"

#ifndef PIPE_DEPTH
#define PIPE_DEPTH 128 /* min 3*/
#endif

#ifndef NUM_MSGS
#define NUM_MSGS 100000L
#endif

#ifdef BENCHMARK
static struct {
  unsigned long msg_cnt;
  double msg_time;
} bench_stats;
#endif

#define STACK_SIZE (16*1024) /* 16k */
#define MSG_TERM ((void*)-1)


typedef struct {
  pthr_stream_t *in, *out;
  int id;
} task_arg_t;

static timing_t ts;


void *Source(void *inarg)
{
  unsigned long cnt = 0;
  pthr_stream_desc_t *out;
  void *msg;

#ifndef BENCHMARK
  printf("Starting message transfer, pipe length %d msgs %lu\n",
      PIPE_DEPTH, NUM_MSGS);
#endif
  TimingStart( &ts);

  out = PthrStreamOpen( (pthr_stream_t *)inarg, 'w');

  while( cnt < (NUM_MSGS-1) ) {
    msg = (void*)(0x10000000 + cnt);
    PthrStreamWrite( out, msg);
    cnt++;
  }

  msg = MSG_TERM;
  PthrStreamWrite( out, msg);

  PthrStreamClose( out, 0);

  pthread_exit(NULL);
}



void *Sink(void *inarg)
{
  unsigned long cnt = 0;
  pthr_stream_desc_t *in;
  void *msg;

  in = PthrStreamOpen( (pthr_stream_t *)inarg, 'r' );

  while(1) {
    msg = PthrStreamRead( in);
    cnt++;
    if (cnt % 10000 == 0) {
      //printf("Read %lu msgs\n", cnt);
    }
    if (msg==MSG_TERM) break;
  }

  PthrStreamClose( in, 1);

  TimingEnd( &ts);

#ifndef BENCHMARK
  printf("End of message stream, cnt %lu duration %.2f ms\n",
      cnt, TimingToMSec(&ts));
#else
  bench_stats.msg_cnt = cnt;
  bench_stats.msg_time = TimingToNSec(&ts);
#endif

  pthread_exit(NULL);
}

void *Relay(void *inarg)
{
  task_arg_t *arg = (task_arg_t *)inarg;
  //int id = arg->id;
  pthr_stream_desc_t *in, *out;
  int term = 0;
  void *msg;

  in = PthrStreamOpen( arg->in, 'r');
  out = PthrStreamOpen( arg->out, 'w');

  while(!term) {
    msg = PthrStreamRead( in);
    if (msg==MSG_TERM) term = 1;
    PthrStreamWrite( out, msg);
  }

  PthrStreamClose( in, 1);
  PthrStreamClose( out, 0);

  free(arg);

  pthread_exit(NULL);
}


static void CreateTask(task_arg_t *arg)
{
  int res;
  pthread_t p;
  pthread_attr_t attr;

  (void) pthread_attr_init( &attr);

  res = pthread_attr_setstacksize(&attr, STACK_SIZE);
  if (res != 0) {
    error(1, res, "Cannot set stack size!");
  }

  res = pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
  if (res != 0) {
    error(1, res, "Cannot set detached!");
  }

  res = pthread_create( &p,
      &attr, Relay, arg);
  if (res != 0) {
    error(1, res, "Cannot create pthread!");
  }
  pthread_attr_destroy( &attr);
}


pthr_stream_t *PipeElement(pthr_stream_t *in, int depth)
{
  pthr_stream_t *out;
  task_arg_t *arg;


  out = PthrStreamCreate();
  arg = (task_arg_t *) malloc( sizeof(task_arg_t));
  arg->in = in;
  arg->out = out;
  arg->id = depth;

  CreateTask(arg);

  //printf("Created Relay %d\n", depth );
  return (depth < (PIPE_DEPTH-2)) ? PipeElement( out, depth+1) : out;
}



static void CreatePipe(void)
{
  pthr_stream_t *glob_in, *glob_out;
  int res;
  pthread_t src, sink;
  pthread_attr_t attr;


  glob_in = PthrStreamCreate();
  glob_out = PipeElement(glob_in, 1);

  (void) pthread_attr_init( &attr);

  res = pthread_attr_setstacksize(&attr, STACK_SIZE);
  if (res != 0) {
    error(1, res, "Cannot set stack size!");
  }


  res = pthread_create( &src, &attr, Source, glob_in);
  if (res != 0) {
    error(1, res, "Cannot create pthread!");
  }

  res = pthread_create( &sink, &attr, Sink, glob_out);
  if (res != 0) {
    error(1, res, "Cannot create pthread!");
  }

  pthread_join( src, NULL);
  pthread_join( sink, NULL);

  pthread_attr_destroy( &attr);
}




static void testBasic(void)
{

  CreatePipe();

}




int main(void)
{
  testBasic();
#ifndef BENCHMARK
  printf("test finished\n");
#else
  printf("%lu %.1f %u\n",
      bench_stats.msg_cnt,
      bench_stats.msg_time,
      PIPE_DEPTH
      );
#endif
  return 0;
}
