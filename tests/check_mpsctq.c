
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "../mpsctq.h"


#define NUM_PROD  20
#define NUM_ITEMS_EACH  2000

static mpsctq_t queue;

/* containing the tasks */
static task_t pool[NUM_PROD*NUM_ITEMS_EACH];

/* a map to insert the received tasks */
static task_t *map[NUM_PROD*NUM_ITEMS_EACH];

void *Consumer(void *arg)
{
  int cnt;
  task_t *t;
  
  cnt = 0;
  while(cnt < NUM_PROD*NUM_ITEMS_EACH) {
    t = MpscTqDequeue(&queue);
    if (t != NULL) {
      //printf("(%p:%lu) ", t, t->uid);
      map[ t - &pool[0] ] = t;
      cnt++;
    }
  }
  return NULL;
}

void *Producer(void *arg)
{
  int id = *( (int*) arg);
  int i, a;
  task_t *t;

  for (i=0; i<NUM_ITEMS_EACH; i++) {
    a = id*NUM_ITEMS_EACH + i;
    assert( 0 <= a && a < NUM_PROD*NUM_ITEMS_EACH);
    t = &pool[ a ];
    t->uid = 1000*(id+1) + i;
    t->next = NULL;
    //printf("(%p:%lu) ", t, t->uid);
    MpscTqEnqueue(&queue, t);
  }
}


int main(int argc, char **argv)
{
  pthread_t tid[NUM_PROD+1];
  int i;
  int ids[NUM_PROD];

  printf("Pool starting address: %p, size of an entry: %#lx\n", pool, sizeof(task_t) );
  printf("Stub address: %p\n", &queue.stub );

  MpscTqInit( &queue );

  (void) memset( &map, 0, NUM_PROD*NUM_ITEMS_EACH );
  pthread_create(&tid[NUM_PROD], NULL, Consumer, NULL);
  for (i=0; i<NUM_PROD; i++) {
    ids[i] = i;
    pthread_create(&tid[i], NULL, Producer, &ids[i]);
  }

  pthread_join(tid[NUM_PROD], NULL);
  for (i=0; i<NUM_PROD; i++) {
    pthread_join(tid[i], NULL);
  }
  
  MpscTqCleanup( &queue );

  for (i=0; i<NUM_PROD*NUM_ITEMS_EACH; i++) {
    assert( map[i] != NULL );
    //printf( "i: %p\n", map[i] );
  }

  return 0;
}
