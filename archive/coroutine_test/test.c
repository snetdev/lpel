
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <malloc.h>
#include <errno.h>
#include <pthread.h>
#include <pcl.h>


#define NUM_TASKS   20
#define NUM_WORKERS  4



struct queue_el {
  struct queue_el *next;
  coroutine_t task;
};



static pthread_mutex_t listlock = PTHREAD_MUTEX_INITIALIZER;

static struct queue_el *head;

void increment(int *val) {
  *val++;
}


void task(void *arg)
{
  int num = *(int *)arg;
  for (;;) {
    printf("Number: %d\n", num);
    //increment(&num);
    co_resume();
  }
}



void *worker(void *arg)
{
  int term = 0;
  struct queue_el *item;

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
    if (pthread_mutex_lock(&listlock) != 0) {
      perror("pthread_mutex_lock error");
      exit(-1);
    }
    // BEGIN CRITICAL SECTION
    item = head;
    if (item != NULL) {
      head = item->next;
    }
    // END CRITICAL SECTION
    if (pthread_mutex_unlock(&listlock) != 0) {
      perror("pthread_mutex_unlock error");
      exit(-1);
    }
    if (item != NULL) {
      printf("Worker %d: ", id);
      co_call(item->task);
      co_delete(item->task);
      free(item);
    } else {
      term = 1;
    }
    sleep(id+1);
    // if NULL, then exit thread
  } while (term==0);
  
  
  co_thread_cleanup();

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

  // create tasks and put in list
  for( i=0; i<NUM_TASKS; i++) {
    struct queue_el *newel = (struct queue_el *) malloc( sizeof(struct queue_el) );
    int *arg = (int *) malloc( sizeof(int) );
    *arg = i;
    newel->task = co_create(task, arg, NULL, 4096);
    newel->next = head;
    head = newel;
  }

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

  co_thread_cleanup();

  return 0;
}
