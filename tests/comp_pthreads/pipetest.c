#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <lpel.h>
#include <lpel/timing.h>

#ifndef PIPE_DEPTH
#define PIPE_DEPTH 100 /* min 3*/
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

#ifndef NUM_WORKERS
#define NUM_WORKERS 2
#endif

#ifndef PART_MOD
#define PART_MOD 1
#endif

#define PARTS       ((NUM_WORKERS)*(PART_MOD))

#ifndef PLACEMENT
#define PLACEMENT   PLACE_PARTS
#endif

#define PLACE_CONST(id)   0
#define PLACE_RR(id)      ((id) % NUM_WORKERS)
#define PLACE_PARTS(id)   (((id) / (PIPE_DEPTH/PARTS)) % NUM_WORKERS)


#define STACK_SIZE (16*1024) /* 16k */
#define MSG_TERM ((void*)-1)

typedef struct {
  lpel_stream_t *in, *out;
  int id;
} task_arg_t;

static lpel_timing_t ts;


void *Source(void *inarg)
{
  unsigned long cnt = 0;
  lpel_stream_desc_t *out;
  void *msg;

#ifndef BENCHMARK
  printf("Starting message transfer, pipe length %d msgs %lu\n", PIPE_DEPTH, NUM_MSGS);
#endif
  LpelTimingStart( &ts);

  out = LpelStreamOpen((lpel_stream_t *)inarg, 'w');

  while( cnt < (NUM_MSGS-1) ) {
    msg = (void*)(0x10000000 + cnt);
    LpelStreamWrite( out, msg);
    cnt++;
  }

  msg = MSG_TERM;
  LpelStreamWrite( out, msg);

  LpelStreamClose( out, 0);
  return NULL;
}



void *Sink(void *inarg)
{
  unsigned long cnt = 0;
  lpel_stream_desc_t *in;
  void *msg;

  in = LpelStreamOpen((lpel_stream_t *)inarg, 'r');

  while(1) {
    msg = LpelStreamRead( in);
    cnt++;
    if (cnt % 10000 == 0) {
      //printf("Read %lu msgs\n", cnt);
    }
    if (msg==MSG_TERM) break;
  }

  LpelStreamClose( in, 1);

  LpelTimingEnd( &ts);
#ifndef BENCHMARK
  printf("End of message stream, cnt %lu duration %.2f ms\n",
      cnt, LpelTimingToMSec(&ts));
#else
  bench_stats.msg_cnt = cnt;
  bench_stats.msg_time = LpelTimingToNSec(&ts);
#endif

  return NULL;
}

void *Relay(void *inarg)
{
  task_arg_t *arg = (task_arg_t *)inarg;
  //int id = arg->id;
  lpel_stream_desc_t *in, *out;
  int term = 0;
  void *msg;

  in = LpelStreamOpen(arg->in, 'r');
  out = LpelStreamOpen(arg->out, 'w');

  while(!term) {
    msg = LpelStreamRead( in);
    if (msg==MSG_TERM) term = 1;
    LpelStreamWrite( out, msg);
  }

  LpelStreamClose( in, 1);
  LpelStreamClose( out, 0);

  free(arg);

  return NULL;
}


static void CreateTask(task_arg_t *arg)
{
  lpel_task_t *t;
  int place;

  place = PLACEMENT(arg->id);

  t = LpelTaskCreate( place, &Relay, arg, STACK_SIZE);
  LpelTaskRun(t);
}


lpel_stream_t *PipeElement(lpel_stream_t *in, int depth)
{
  lpel_stream_t *out;
  task_arg_t *arg;


  out = LpelStreamCreate(0);
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
  lpel_stream_t *glob_in, *glob_out;
  lpel_task_t *t;


  glob_in = LpelStreamCreate(0);
  glob_out = PipeElement(glob_in, 1);

  t = LpelTaskCreate( 0, &Source, glob_in, STACK_SIZE);
  LpelTaskRun(t);

  t = LpelTaskCreate( 1, &Sink, glob_out, STACK_SIZE);
  LpelTaskRun(t);
}



static void testBasic(void)
{
  lpel_config_t cfg;
  memset(&cfg, 0, sizeof(lpel_config_t));

  cfg.num_workers = NUM_WORKERS;
  cfg.proc_workers = NUM_WORKERS;
  cfg.proc_others = 0;
  cfg.flags = 0;

  LpelInit(&cfg);

  LpelStart();
  CreatePipe();

  LpelStop();
  LpelCleanup();
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

