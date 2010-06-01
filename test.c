

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <sys/types.h>
#include <linux/unistd.h>
#include <sys/syscall.h>

#include "stream.h"


#define handle_error_en(en, msg) \
  do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)


#define gettid() syscall( __NR_gettid )


#ifdef ASSIGN_CORE
#define assign_core(cid) \
  do { \
    int res; \
    cpu_set_t cpuset; \
    pthread_t thread; \
    pid_t tid; \
    struct sched_param param; \
    thread = pthread_self(); \
    tid = gettid(); \
    CPU_ZERO(&cpuset); \
    /*TODO: check for range */ \
    CPU_SET( (cid) , &cpuset); \
    /*res = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);*/ \
    res = sched_setaffinity(tid, sizeof(cpu_set_t), &cpuset); \
    if (res != 0) { \
      /* handle_error_en(res, "pthread_setaffinity_np");*/ \
      handle_error_en(errno, "sched_setaffinity"); \
    } \
    /*TODO check permissions */\
    param.sched_priority = 1; /* lowest real-time */ \
    /* following can also be SCHED_FIFO */\
    /* res = pthread_setschedparam(thread, SCHED_RR, &param);*/ \
    res = sched_setscheduler(tid, SCHED_FIFO, &param); \
    if (res != 0) { \
      /*handle_error_en(res, "pthread_setschedparam");*/ \
      handle_error_en(errno, "sched_setscheduler"); \
    } \
  } while (0)
#else 
#define assign_core(cid) ;/*nop*/
#endif


static const unsigned int max_items = 200;

static stream_t *s;

void *producer(void *arg)
{
  unsigned long i;

  assign_core(0);

  for (i=1; i<=max_items; i++) {
    StreamWrite(s, (void *) i);
    printf("w%ld. ", i);
  }
  printf("Producer finished. \n");
  fflush(stdout);
  
  return NULL;
}

void *consumer(void *arg)
{
  unsigned long i,j;

  assign_core(1);

  for (i=1; i<=max_items; i++) {
    j = (unsigned long) StreamRead(s);
    printf("(%ld) ", j);
  }
  printf("Consumer finished. \n");
  fflush(stdout);
  return NULL;
}


int main(int argc, char **argv)
{
  pthread_t th_p, th_c;
  int numcpus;
  /* query the number of cpus */
  numcpus = sysconf(_SC_NPROCESSORS_ONLN);
  printf("Number of CPUs: %d\n", numcpus);

  s = StreamCreate();

  pthread_create( &th_p, NULL, producer, NULL );
  pthread_create( &th_c, NULL, consumer, NULL );

  pthread_join( th_p, NULL );
  pthread_join( th_c, NULL );

  StreamDestroy(s);

  exit(EXIT_SUCCESS);
}
