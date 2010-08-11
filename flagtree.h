#ifndef _FLAGTREE_H_
#define _FLAGTREE_H_

/* number of nodes for a given height h: 2^(h+1)-1*/
#define NODES(h)  (1<<((h)+1) - 1)

/* number of leafs for a given height h: 2^h */
#define LEAFS(h)  (1<<(h))

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
#define MAP(h,i)  ( (i) + (1<<(h)) - 1 )

/* inverse function of MAP: i+1 - 2^h */
#define MAP_INV(h,i)  ( (i)+1 - (1<<(h)) )

/* parent index of index i: floor( (i-1)/2 ) */
#define PARENT(i)   ( ((i)-1)/2 )

/* left child index of index i: 2i+1 */
#define LEFT(i)   ( 2*(i) + 1 )

/* right child index of index i: 2i+2 = 2(i+1)*/
#define RIGHT(i)  ( 2*((i)+1) )


typedef struct {
  int height;
  int *buf;
} flagtree_t;


/**
 * mark the tree up to the root starting with leaf #idx,
 * writing value val
 *
 * @pre 0 <= idx < LEAFS(heap.height) [=2^height]
 */
static inline void flagtree_mark(flagtree_t *heap, int idx, int val)
{
  int j = MAP(heap->height, idx);
  heap->buf[j] = val;
  while (j != 0) {
    j = PARENT(j);
    heap->buf[j] = val;
  }
}



#endif /* _FLAGTREE_H_ */
