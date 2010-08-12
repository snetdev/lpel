/**
 * Implements a flag signalling tree for Collector tasks
 *
 * Assumption: FlagtreeGrow and FlagtreeGather are never
 *             called concurrently on the same flagtree!
 */


#include <stdlib.h>

/* FlagtreePrint will only be included if NDEBUG is not set */
#ifndef NDEBUG
#include <stdio.h>
#include <string.h>
#endif

#include "flagtree.h"


static void (*callback_gather)(int);

/**
 * Allocate the ft for a given height
 */
void FlagtreeAlloc(flagtree_t *ft, int height)
{
  ft->height = height;
  ft->buf = (int *) calloc( FT_NODES(height), sizeof(int) );
}

/**
 * Free the ft
 */
void FlagtreeFree(flagtree_t *ft)
{
  ft->height = -1;
  free(ft->buf);
}

/**
 * Grow the ft by one level
 */
void FlagtreeGrow(flagtree_t *ft)
{
  int *old_buf, *new_buf;
  int old_height;
  int i;

  /* store the old values */
  old_height = ft->height;
  old_buf = ft->buf;

  /* allocate space */
  new_buf = (int *) calloc( FT_NODES(old_height+1), sizeof(int) );

  //LOCK WRITE
  ft->height += 1;
  ft->buf = new_buf;
  
  /* set the appropriate flags */
  for (i=0; i<FT_LEAFS(old_height); i++) {
    /* test each leaf in the old ft */
    if ( old_buf[FT_LEAF_TO_IDX(old_height, i)] == 1 ) {
      /* mark this leaf in the new ft as well */
      FlagtreeMark( ft, i );
    }
  }
  //UNLOCK WRITE

  /* free the old ft space */
  free(old_buf);
}

/**
 * @pre idx is marked
 */
static void Visit(flagtree_t *ft, int idx)
{
  /* preorder: clear current node */
  ft->buf[idx] = 0;
  if ( idx < FT_LEAF_START_IDX(ft->height) ) {
    /* if inner node, descend: */
    /* left child */
    if (ft->buf[FT_LEFT_CHILD(idx)] != 0) {
      Visit(ft, FT_LEFT_CHILD(idx));
    }
    /* right child */
    if (ft->buf[FT_RIGHT_CHILD(idx)] != 0) {
      Visit(ft, FT_RIGHT_CHILD(idx));
    }
  } else {
    /* gather leaf of idx */
    callback_gather( FT_IDX_TO_LEAF(ft->height, idx) );
  }
}

/**
 * Gather marked leafs recursively,
 * clearing the marks in preorder
 */
void FlagtreeGatherRec(flagtree_t *ft, void (*gather)(int) )
{
  /* no locking necessary, as only the gatherer will grow the tree */
  callback_gather = gather;
  /* start from root */
  Visit(ft, 0);
};


#ifndef NDEBUG
/**
 * Print a flagtree to stderr
 */
void FlagtreePrint(flagtree_t *ft)
{
  int lvl, i, j;
  int maxpad = (1<<(ft->height+1))-2;
  int curpad;
  char pad[maxpad+1];

  memset(pad, (int)' ', maxpad);
  curpad = maxpad;
  i=0;
  for (lvl=0; lvl<=ft->height; lvl++) {
    pad[ curpad ] = '\0';
    for (j=0; j<FT_NODES_AT_LEVEL(lvl); j++) {
      fprintf(stderr,"%s", pad);
      if (ft->buf[i] != 0) {
        fprintf(stderr,"(%2d)",i);
      } else {
        fprintf(stderr," %2d ",i);
      }
      fprintf(stderr,"%s",pad);
      i++;
    }
    curpad = curpad - (1<<(ft->height-lvl));
    fprintf(stderr,"\n\n");
  }
}

#endif
