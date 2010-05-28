
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <malloc.h>
#include <errno.h>
#include <pthread.h>
#include <pcl.h>


#define COUNTER_MAX 3
#define NUM_WORKERS 4



static coroutine_t tasks[2];

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

// declaration
static void counter_task(void *arg);



static void creator_task(void *arg)
{
    coroutine_t created_task = co_create(counter_task, NULL, NULL, 4096);

    // place the created coroutine in the tasks[] array
    tasks[1] = created_task;
    printf("Creator Task created and placed counter!\n");

    co_resume();
}

static void counter_task(void *arg)
{
  int counter=0;
  for (;;) {
    printf("Counter Task: %d\n", counter);
    counter++;
    co_resume();
  }
}



void *worker(void *arg)
{
  int term = COUNTER_MAX;

  int id = *(int *)arg;
/*
  if (co_thread_init() < 0) {
    perror("co_thread_init failed in worker\n");
    exit(-1);
  }
  */
  printf("Created worker %d\n", id);

  do {
    // mutual exclusion on array
    if (pthread_mutex_lock(&lock) != 0) {
      perror("pthread_mutex_lock error");
      exit(-1);
    }
    // BEGIN CRITICAL SECTION
    if (tasks[1] == NULL) {
      printf("Worker %d calling task[0]=creator (term=%d)\n", id, term);
      co_call(tasks[0]);
    } else {
      printf("Worker %d calling task[1]=counter (term=%d)\n", id, term);
      co_call(tasks[1]);
    }
    // END CRITICAL SECTION
    if (pthread_mutex_unlock(&lock) != 0) {
      perror("pthread_mutex_unlock error");
      exit(-1);
    }
    printf("Worker %d unlock and sleep\n", id);

    sleep(1);
    term--;
  } while (term>0);
  
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


  tasks[0] = co_create(creator_task, NULL, NULL, 4096);
  tasks[1] = NULL;
  

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
  
  co_delete(tasks[0]);
  co_delete(tasks[1]);
  
  co_thread_cleanup();

  return 0;
}
