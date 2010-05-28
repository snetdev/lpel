
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <malloc.h>
#include <errno.h>
#include <pthread.h>
#include <pcl.h>


#define COUNTER_MAX 50
#define NUM_WORKERS 3

#define NUM_TASKS_IN_QUEUE 5 // <100

static coroutine_t queue[NUM_WORKERS][NUM_TASKS_IN_QUEUE];



static void counter_task(void *arg)
{
  // counter initialized with id
  int counter= (int) arg;
  for (;;) {
    printf("counter task: %d\n", counter);
    counter++;
    co_resume();
  }
}



void *worker(void *arg)
{
  int term = COUNTER_MAX;

  int id = (int)arg;
  int idx = 0;
  /*
  if (co_thread_init() < 0) {
    perror("co_thread_init failed in worker\n");
    exit(-1);
  }
  */
  printf("Created worker %d\n", id);

  do {
    // pull task from dedicated runqueue
    printf("Worker %d (term=%3d): ", id, term);
    co_call(queue[id][idx]);
    idx = (idx+1) % NUM_TASKS_IN_QUEUE;
    sleep( id+1 );
    term--;
  } while (term>0);
  
  printf("Finished worker %d\n", id);
  
  //co_thread_cleanup();

  return 0;
}


int main(int argc, char **argv)
{
  pthread_t *thids;
  int i, j;

  if (co_thread_init() < 0) {
    perror("co_thread_init failed in main\n");
    exit(-1);
  }

  // fill the task-queues
  for (i=0; i<NUM_WORKERS; i++) {
    for(j=0; j<NUM_TASKS_IN_QUEUE; j++) {
      queue[i][j] = co_create(counter_task, (void *)((i+1)*1000+(j+1)*100), NULL, 4096);
    }
  }
  

  // launch worker threads
  thids = (pthread_t *) malloc(NUM_WORKERS * sizeof(pthread_t));
  for (i = 0; i < NUM_WORKERS; i++) {
    if (pthread_create(&thids[i], NULL, worker, (void *)i)) {
      perror("creating worker threads");
      exit(-1);
    }
  }
  // join on finish
  for (i = 0; i < NUM_WORKERS; i++)
    pthread_join(thids[i], NULL);
  
  // delete tasks from queues
  for (i=0; i<NUM_WORKERS; i++) {
    for(j=0; j<NUM_TASKS_IN_QUEUE; j++) {
      co_delete( queue[i][j] );
    }
  }
  
  co_thread_cleanup();

  return 0;
}
