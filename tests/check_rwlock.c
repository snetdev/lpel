
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "../rwlock.h"


#define NUM_READERS   20

static rwlock_t lock;

static volatile int control;
static volatile int in_read;
static volatile int shared;


void *writer(void *arg)
{
  int i;
  while(1) {
    rwlock_writer_lock(&lock);
    assert( in_read == 0 );
    for (i=0;i<=0x4e20;i++) {
      shared = shared + rand();
      printf("%d ",shared);
    }
    //sleep(1);
    control = shared;
    fprintf(stderr, "[%d]\n", control);
    rwlock_writer_unlock(&lock);
  }
  return NULL;
}

void *reader(void *arg)
{
  int id = *( (int*) arg);
  while(1) {
    rwlock_reader_lock(&lock, id);
    in_read = 1;
    assert( shared == control );
    in_read = 0;
    rwlock_reader_unlock(&lock, id);
  }
}


int main(int argc, char **argv)
{
  pthread_t tid[NUM_READERS+1];
  int i;
  int ids[NUM_READERS];


  srand(0x1383);
  shared = control = rand();
  rwlock_init(&lock, NUM_READERS);

  pthread_create(&tid[NUM_READERS], NULL, writer, NULL);
  for (i=0; i<NUM_READERS; i++) {
    ids[i] = i;
    pthread_create(&tid[i], NULL, reader, &ids[i]);
  }

  pthread_join(tid[NUM_READERS], NULL);
  for (i=0; i<NUM_READERS; i++) {
    pthread_join(tid[i], NULL);
  }
  rwlock_cleanup(&lock);
  return 0;
}
