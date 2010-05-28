
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <malloc.h>
#include <errno.h>
#include <pthread.h>
#include <pcl.h>


#define COUNTER_MAX 20
#define NUM_WORKERS  4



static coroutine_t task;


static int counter = 0;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;



void count_inc(void *arg)
{
  for (;;) {
    printf("Counter: %d\n", counter);
    counter++;
    co_resume();
  }
}



void *worker(void *arg)
{
  int term = 0;

  int id = *(int *)arg;
/*
  if (co_thread_init() < 0) {
    perror("co_thread_init failed in worker\n");
    exit(-1);
  }
  */
  printf("Created worker %d\n", id);

  do {
    // mutual exclusion pull task from queue
    if (pthread_mutex_lock(&lock) != 0) {
      perror("pthread_mutex_lock error");
      exit(-1);
    }
    // BEGIN CRITICAL SECTION
    printf("Worker %d calling task\n", id);
    co_call(task);
    printf("Worker %d reads counter %d\n", id, counter);
    if (counter >= COUNTER_MAX) {
      term = 1;
    }
    // END CRITICAL SECTION
    if (pthread_mutex_unlock(&lock) != 0) {
      perror("pthread_mutex_unlock error");
      exit(-1);
    }
    printf("Worker %d unlock and sleep\n", id);
    sleep(1);
    
  } while (term==0);
  
  //co_thread_cleanup();

  return 0;
}


int main(int argc, char **argv)
{
  pthread_t *thids;
  int i;

  if (co_thread_init() < 0) {
    perror("co_thread_init failed in main\n");
    exit(-1);
  }


  task = co_create(count_inc, NULL, NULL, 4096);
  

  // launch worker threads
  thids = (pthread_t *) malloc(NUM_WORKERS * sizeof(pthread_t));
  for (i = 0; i < NUM_WORKERS; i++) {
    int *arg = (int *) malloc( sizeof(int) );
    *arg = i;
    if (pthread_create(&thids[i], NULL, worker, arg)) {
      perror("creating worker threads");
      exit(-1);
    }
  }
  // join on finish
  for (i = 0; i < NUM_WORKERS; i++)
    pthread_join(thids[i], NULL);
  
  co_delete(task);
  
  co_thread_cleanup();

  return 0;
}
