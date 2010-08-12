
#include <stdlib.h>

#ifndef NDEBUG
#include <stdio.h>
#include <string.h>
#endif

#include "flagtree.h"


static void (*callback_gather)(int);

/**
 * Allocate the heap for a given height
 */
void FlagtreeAlloc(flagtree_t *heap, int height)
{
  heap->height = height;
  heap->buf = (int *) calloc( NODES(height), sizeof(int) );
}

/**
 * Free the heap
 */
void FlagtreeFree(flagtree_t *heap)
{
  heap->height = -1;
  free(heap->buf);
}

/**
 * Grow the heap by one level
 */
void FlagtreeGrow(flagtree_t *heap)
{
  int *old_buf, *new_buf;
  int old_height;
  int i;

  /* store the old values */
  old_height = heap->height;
  old_buf = heap->buf;

  /* allocate space */
  new_buf = (int *) calloc( NODES(old_height+1), sizeof(int) );

  //LOCK
  heap->height += 1;
  heap->buf = new_buf;
  
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
    callback_gather( IDX_TO_LEAF(heap->height, idx) );
  }
}

/**
 * Gather marked leafs recursively,
 * clearing the marks in preorder
 */
void FlagtreeGatherRec(flagtree_t *heap, void (*gather)(int) )
{
  callback_gather = gather;
  /* start from root */
  Visit(heap, 0);
};


#ifndef NDEBUG
/**
 * Print a flagtree to stderr
 */
void FlagtreePrint(flagtree_t *heap)
{
  int lvl, i, j;
  int maxpad = (1<<(heap->height+1))-2;
  int curpad;
  char pad[maxpad+1];

  memset(pad, (int)' ', maxpad);
  curpad = maxpad;
  i=0;
  for (lvl=0; lvl<=heap->height; lvl++) {
    pad[ curpad ] = '\0';
    for (j=0; j<NODES_AT_LEVEL(lvl); j++) {
      fprintf(stderr,"%s", pad);
      if (heap->buf[i] != 0) {
        fprintf(stderr,"(%2d)",i);
      } else {
        fprintf(stderr," %2d ",i);
      }
      fprintf(stderr,"%s",pad);
      i++;
    }
    curpad = curpad - (1<<(heap->height-lvl));
    fprintf(stderr,"\n\n");
  }
}

#endif
