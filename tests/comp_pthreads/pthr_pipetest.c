#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <error.h>
#include <pthread.h>
#include <assert.h>

#include "pthr_streams.h"

#include "arch/timing.h"


#define STACK_SIZE (16*1024) /* 16k */

#define PIPE_DEPTH 100 /* min 3*/

#define NUM_MSGS 100000L

#define MSG_TERM ((void*)-1)

typedef struct {
  pthr_stream_t *in, *out;
  int id;
} task_arg_t;

static timing_t ts;


void *Source(void *inarg)
{
  unsigned long cnt = 0;
  pthr_stream_t *out;
  void *msg;

  printf("Starting message transfer, pipe length %d msgs %lu\n",
      PIPE_DEPTH, NUM_MSGS);
  TimingStart( &ts);

  out = (pthr_stream_t *)inarg;

  while( cnt < (NUM_MSGS-1) ) {
    msg = (void*)(0x10000000 + cnt);
    PthrStreamWrite( out, msg);
    cnt++;
  }

  msg = MSG_TERM;
  PthrStreamWrite( out, msg);

  pthread_exit(NULL);
}



void *Sink(void *inarg)
{
  unsigned long cnt = 0;
  pthr_stream_t *in;
  void *msg;

  in = (pthr_stream_t *)inarg;

  while(1) {
    msg = PthrStreamRead( in);
    cnt++;
    if (cnt % 10000 == 0) {
      //printf("Read %lu msgs\n", cnt);
    }
    if (msg==MSG_TERM) break;
  }

  PthrStreamDestroy( in);

  TimingEnd( &ts);
  printf("End of message stream, cnt %lu duration %.2f ms\n",
      cnt, TimingToMSec(&ts));

  pthread_exit(NULL);
}

void *Relay(void *inarg)
{
  task_arg_t *arg = (task_arg_t *)inarg;
  //int id = arg->id;
  pthr_stream_t *in, *out;
  int term = 0;
  void *msg;

  in = arg->in;
  out = arg->out;
  
  while(!term) {
    msg = PthrStreamRead( in);
    if (msg==MSG_TERM) term = 1;
    PthrStreamWrite( out, msg);
  }

  PthrStreamDestroy( in);

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
  printf("test finished\n");
  return 0;
}
