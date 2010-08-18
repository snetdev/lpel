#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <assert.h>

#include "../flagtree.h"


flagtree_t H;
rwlock_t lock;

static void header(const char *s);


void GatherLeaf(int i, void *arg)
{
  fprintf(stderr,"gather %d\n",i);
}

static void testAlloc(void)
{
  int i;

  header("Allocate"); 
  for (i=0; i<=4; i++) {
    FlagtreeAlloc(&H, i, &lock);
    fprintf(stderr,"height %d nodes %d\n", H.height, FT_NODES(H.height));
    FlagtreePrint(&H);
    FlagtreeFree(&H);
  }
}


static void testMark(void)
{
  int leaf, i;
  int num_marks = 3;
  int marks[] = { 3,0,5 };

  header("Mark");
  FlagtreeAlloc(&H, 3, &lock);
  for (i=0; i<num_marks; i++) {
    leaf=marks[i];
    fprintf(stderr,"marking leaf %d (=idx %d)\n", leaf, FT_LEAF_TO_IDX(H.height,leaf));
    FlagtreeMark(&H, leaf, 0);
    FlagtreePrint(&H);
  }
  FlagtreeFree(&H);
}

static void testGrow(void)
{
  int leaf, i;
  int num_marks = 3;
  int marks[] = { 3,0,5 };

  header("Grow");
  FlagtreeAlloc(&H, 3, &lock);

  for (i=0; i<num_marks; i++) {
    leaf=marks[i];
    FlagtreeMark(&H, leaf, 0);
  }
  FlagtreePrint(&H);

  fprintf(stderr,"Grow + mark 14\n");
  FlagtreeGrow(&H);
  FlagtreeMark(&H, 14, 0);
  FlagtreePrint(&H);

  FlagtreeFree(&H);
}

static void testGather(void)
{
  int leaf, i;
  int num_marks = 5;
  int marks[] = { 3,0,5,14,1 };

  header("Gather");
  FlagtreeAlloc(&H, 4, &lock);

  for (i=0; i<num_marks; i++) {
    leaf=marks[i];
    FlagtreeMark(&H, leaf, 0);
  }
  FlagtreePrint(&H);

  fprintf(stderr,"Gather\n");
  FlagtreeGather(&H, GatherLeaf, NULL);
  FlagtreePrint(&H);

  FlagtreeFree(&H);
}

int main(void)
{

  rwlock_init( &lock, 1 );

  testAlloc();
  testMark();
  testGrow();
  testGather();

  rwlock_cleanup( &lock );
  return 0;
}


/*** HELPER *****************************************************************/

static void header(const char *s)
{
  int i,len = strlen(s);
  fprintf(stderr,"=== %s ", s);
  for (i=0; i<(79-len-5); i++) fprintf(stderr,"=");
  fprintf(stderr,"\n");
}


