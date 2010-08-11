
#include <stdlib.h>
#include <stdio.h>

#include "flagtree.h"


#define GATHER_LEAF(i)  /*TODO*/


/**
 * Allocate the heap for a given height
 */
void FlagtreeAlloc(flagtree_t *heap, int height)
{
  heap->height = height;
  heap->buf = (int *) calloc( NODES(height), sizeof(int) );
}

/**
 * Grow the heap by one level
 */
void FlagtreeGrow(flagtree_t *heap)
{
  int *old_heap, *new_heap;
  int old_height;
  int i;

  /* store the old values */
  old_height = heap->height;
  old_buf = heap->buf;

  /* allocate space */
  new_heap = (int *) calloc( NODES(old_height+1), sizeof(int) );

  //LOCK
  heap->height += 1;
  heap->buf = new_heap;
  
  /* set the appropriate flags */
  for (i=0; i<LEAFS(old_height); i++) {
    /* test each leaf in the old heap */
    if ( old_buf[LEAF_TO_IDX(old_height, i)] == 1 ) {
      /* mark this leaf in the new heap as well */
      FlagtreeMark( heap, i );
    }
  }
  //UNLOCK

  /* free the old heap space */
  free(old_buf);
}

/**
 * @pre idx is marked
 */
static void Visit(flagtree_t *heap, int idx)
{
  /* preorder: clear current node */
  heap->buf[idx] = 0;
  if ( idx < LEAF_START_IDX(heap->height) ) {
    /* if inner node, descend: */
    /* left child */
    if (heap->buf[LEFT(idx)] != 0) {
      Visit(heap, LEFT(idx));
    }
    /* right child */
    if (heap->buf[RIGHT(idx)] != 0) {
      Visit(heap, RIGHT(idx));
    }
  } else {
    /* gather leaf of idx */
    GATHER_LEAF( IDX_TO_LEAF(heap->height, idx) );
  }
}

/**
 * TODO make gathering iterative
 */
void FlagtreeGather(flagtree_t *heap)
{
  /* start from root */
  Visit(heap, 0);
}


void FlagtreePrint(flagtree_t *heap)
{
  int lvl, i, j;
  char *pad;

  i=0;
  for (lvl=0; lvl<=heap->height; lvl++) {
    for (j=0; j<NODES_AT_LEVEL(lvl); j++) {
      printf(pad);
      if (heap->buf[i] != 0) {
        printf("(%2d)",i);
      } else {
        printf(" %2d ",i);
      }
      printf(pad);
      i++;
    }
    printf("\n");
  }
}
