
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <malloc.h>
#include <errno.h>
#include <pthread.h>
#include <pcl.h>


#define NUM_TASKS   20
#define NUM_WORKERS  2


typedef struct {
  int num;
  coroutine_t caller;
} taskarg_t;

struct node {
  struct node *next;
  coroutine_t task;
  taskarg_t *arg;
};

typedef struct node node_t;

static pthread_mutex_t listlock = PTHREAD_MUTEX_INITIALIZER;

void lock(void) {
  if (pthread_mutex_lock(&listlock) != 0) {
    perror("pthread_mutex_lock error");
    exit(-1);
  }
}

void unlock(void) {
  if (pthread_mutex_unlock(&listlock) != 0) {
    perror("pthread_mutex_unlock error");
    exit(-1);
  }
}


static node_t *handle;

void append_node( node_t *n)
{
  lock();
  if (handle == NULL) {
    handle = n;
    n->next = n;
  } else {
    n->next = handle->next;
    handle->next = n;
    handle = n;
  }
  unlock();
}

node_t *remove_node( void)
{
  node_t *n = NULL;

  lock();
  if (handle != NULL) {
    n = handle->next;
    if (n == handle) {
      handle = NULL;
    } else {
      handle->next = n->next;
    }
  }
  unlock();
  return n;
}


void task(void *arg)
{
  taskarg_t *a = (taskarg_t *)arg;
  for (;;) {
    printf("(%d) ", a->num);
    co_call(a->caller);
  }
}



void *worker(void *arg)
{
  int term = 0;
  node_t *item;
  coroutine_t wctx;

  int id = *(int *)arg;
  if (co_thread_init() < 0) {
    perror("co_thread_init failed in worker\n");
    exit(-1);
  }
  printf("Created worker %d\n", id);

  wctx = co_current();
  do {
    item = remove_node();

    if (item != NULL) {
      printf("\nWorker %d: ", id);
      item->arg->caller = wctx;
      co_call(item->task);

      append_node(item);

      //co_delete(item->task);
      //free(item);
    } else {
      //term = 1;
    }
    //sleep(id+1);
    // if NULL, then exit thread
  } while (term==0);
  
  printf("Worker exited %d\n", id);
  
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

  handle = NULL;

  // create tasks and put in list
  for( i=0; i<NUM_TASKS; i++) {
    node_t *n = (node_t *) malloc( sizeof( node_t) );
    taskarg_t *arg = (taskarg_t *) malloc( sizeof(taskarg_t) );
    arg->num = i;

    n->task = co_create(task, arg, NULL, 4096);
    n->arg  = arg;

    append_node( n);
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
