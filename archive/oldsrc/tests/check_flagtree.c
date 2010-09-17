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

static void testInit(void)
{
  int i;

  header("Allocate"); 
  for (i=0; i<=4; i++) {
    FlagtreeInit(&H, i, &lock);
    fprintf(stderr,"height %d nodes %d\n", H.height, FT_NODES(H.height));
    FlagtreePrint(&H);
    FlagtreeCleanup(&H);
  }
}


static void testMark(void)
{
  int leaf, i;
  int num_marks = 3;
  int marks[] = { 3,0,5 };

  header("Mark");
  FlagtreeInit(&H, 3, &lock);
  for (i=0; i<num_marks; i++) {
    leaf=marks[i];
    fprintf(stderr,"marking leaf %d (=idx %d)\n", leaf, FT_LEAF_TO_IDX(H.height,leaf));
    FlagtreeMark(&H, leaf, 0);
    FlagtreePrint(&H);
  }
  FlagtreeCleanup(&H);
}

static void testGrow(void)
{
  int leaf, i;
  int num_marks = 3;
  int marks[] = { 3,0,5 };

  header("Grow");
  FlagtreeInit(&H, 3, &lock);

  for (i=0; i<num_marks; i++) {
    leaf=marks[i];
    FlagtreeMark(&H, leaf, 0);
  }
  FlagtreePrint(&H);

  fprintf(stderr,"Grow + mark 14\n");
  FlagtreeGrow(&H);
  FlagtreeMark(&H, 14, 0);
  FlagtreePrint(&H);

  FlagtreeCleanup(&H);
}

static void testGather(void)
{
  int leaf, i;
  int num_marks = 5;
  int marks[] = { 3,0,5,14,1 };
  int cnt;

  header("Gather");
  FlagtreeInit(&H, 4, &lock);

  for (i=0; i<num_marks; i++) {
    leaf=marks[i];
    FlagtreeMark(&H, leaf, 0);
  }

  /* delete a leaf */
  //H.buf[18] = 0;
  
  FlagtreePrint(&H);

  fprintf(stderr,"Gather\n");
  cnt = FlagtreeGather(&H, GatherLeaf, NULL);
  //cnt = FlagtreeGatherRec(&H, GatherLeaf, NULL);
  fprintf(stderr,"= %d\n", cnt);
  assert( cnt == num_marks );
  FlagtreePrint(&H);

  FlagtreeCleanup(&H);
}

int main(void)
{

  RwlockInit( &lock, 1 );

  testInit();
  testMark();
  testGrow();
  testGather();

  RwlockCleanup( &lock );
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


