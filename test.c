

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#include "stream.h"


#define handle_error_en(en, msg) \
  do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)


#ifdef ASSIGN_CORE
#define assign_core(cid) \
  do { \
    int res; \
    cpu_set_t cpuset; \
    pthread_t thread; \
    struct sched_param param; \
    thread = pthread_self(); \
    CPU_ZERO(&cpuset); \
    /*TODO: check for range */ \
    CPU_SET( (cid) , &cpuset); \
    res = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset); \
    if (res != 0) { \
      handle_error_en(res, "pthread_setaffinity_np"); \
    } \
    /*TODO check permissions */\
    param.sched_priority = 1; /* lowest real-time */ \
    /* following can also be SCHED_FIFO */\
    res = pthread_setschedparam(thread, SCHED_RR, &param); \
    if (res != 0) { \
      handle_error_en(res, "pthread_setschedparam"); \
    } \
  } while (0)
#else 
#define assign_core(cid) ;/*nop*/
#endif




static stream_t *s;

void *producer(void *arg)
{
  unsigned int i;

  assign_core(0);

  for (i=1; i<=100; i++) {
    StreamWrite(s, (void *) i);
  }
  printf("Producer finished. \n");
  fflush(stdout);
  
  return NULL;
}

void *consumer(void *arg)
{
  unsigned int i,j;

  assign_core(1);

  for (i=1; i<=100; i++) {
    j = (unsigned int) StreamRead(s);
    printf("(%d) ", j);
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
