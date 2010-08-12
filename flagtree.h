#ifndef _FLAGTREE_H_
#define _FLAGTREE_H_

/* number of nodes for a given height h: 2^(h+1)-1*/
#define NODES(h)  ( (1<<((h)+1)) - 1 )

/* number of leafs for a given height h: 2^h */
#define LEAFS(h)  (1<<(h))

/* number of nodes at a certain level v (root=0): 2^v */
#define NODES_AT_LEVEL(v)   (1<<(v))

/* number of flags to set for a given height h: h+1 */
#define CNT_FLAGS(h)  ((h)+1)

/* which index of the tree the leafs start for a given height h: 2^h - 1 */
#define LEAF_START_IDX(h) ( (1<<(h)) - 1 )


/* mapping from an index i of the leafs
 * to the index in the heap, depending on height h:
 * i + (2^h - 1)
 * e.g.
 *  leaf #0 -> index 7  if h=3
 *  leaf #3 -> index 10 if h=3
 */
#define LEAF_TO_IDX(h,i)  ( (i) + (1<<(h)) - 1 )

/* inverse function of LEAF_TO_IDX: i+1 - 2^h */
#define IDX_TO_LEAF(h,i)  ( (i)+1 - (1<<(h)) )

/* parent index of index i: floor( (i-1)/2 ) */
#define PARENT(i)   ( ((i)-1)/2 )

/* left child index of index i: 2i+1 */
#define LEFT(i)   ( 2*(i) + 1 )

/* right child index of index i: 2i+2 = 2(i+1)*/
#define RIGHT(i)  ( 2*((i)+1) )



/**
 * This function has to be defined by the including module
 */

typedef struct {
  int height;
  int *buf;
} flagtree_t;


/**
 * mark the tree up to the root starting with leaf #idx
 *
 * @pre 0 <= idx < LEAFS(heap.height) [=2^height]
 */
static inline void FlagtreeMark(flagtree_t *heap, int idx)
{
  int j = LEAF_TO_IDX(heap->height, idx);
  heap->buf[j] = 1;
  while (j != 0) {
    j = PARENT(j);
    heap->buf[j] = 1;
  }
}


/**
 * Gather marked leafs iteratively,
 * clearing the marks in preorder
 */
static inline void FlagtreeGather(flagtree_t *heap, void (*gather)(int) )
{
  int prev, cur, next;
  /* start from root */
  prev = cur = 0;
  do {
    /* for stepwise debugging: */
#ifndef NDEBUG
    /* FlagtreePrint(heap); */
#endif
    if (prev == PARENT(cur)) {
      /* clear cur */
      heap->buf[cur] = 0;
      if ( cur < LEAF_START_IDX(heap->height) ) {
        if ( heap->buf[LEFT(cur)] != 0 ) { next = LEFT(cur); }
        else if ( heap->buf[RIGHT(cur)] != 0 ) { next = RIGHT(cur); }
        else { next = PARENT(cur); }
      } else {
        /* gather leaf of idx */
        gather( IDX_TO_LEAF(heap->height, cur) );
        next = PARENT(cur);
      }

    } else if (prev == LEFT(cur)) {
      if ( heap->buf[RIGHT(cur)] != 0 ) { next = RIGHT(cur); }
      else { next = PARENT(cur); }

    } else if (prev == RIGHT(cur)) {
      next = PARENT(cur);
    }
    prev = cur;
    cur = next;;
  } while (cur != prev);
}



extern void FlagtreeAlloc(flagtree_t *heap, int height);
extern void FlagtreeFree(flagtree_t *heap);
extern void FlagtreeGrow(flagtree_t *heap);
extern void FlagtreeGatherRec(flagtree_t *heap, void (*gather)(int) );

#ifndef NDEBUG
extern void FlagtreePrint(flagtree_t *heap);
#endif

#endif /* _FLAGTREE_H_ */
