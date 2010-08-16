#ifndef _STREAMSET_H_
#define _STREAMSET_H

#include <stdio.h>


#define STREAMSET_GRP_SIZE  4



/* for convenience */
struct stream;


/**
 * stream table entry
 */
struct streamtbe {
  struct stream *s;
  char state;
  unsigned long cnt;
  struct streamtbe *dirty;  /* for chaining 'dirty' entries */
};

typedef struct streamtbe streamtbe_t;

struct streamgrp {
  struct streamgrp *next;  /* for the chain */
  streamtbe_t tab[STREAMSET_GRP_SIZE];
};


typedef struct {
  int grp_capacity;
  struct streamgrp **lookup;  /* array of pointers to the grps */
  int idx_grp;                /* current grp for next free entry */
  int idx_tab;                /* current index of the tab within
                                 the grp for next free entry */
  int cnt_obsolete;           /* number of obsolete table entries */
  streamtbe_t *dirty_list;    /* start of 'dirty' list */
  struct {
    struct streamgrp *hnd;    /*   handle to the chain */
    int count;                /*   total number of chained table entries */
  } chain;                    /* chain of groups */
} streamset_t;

typedef struct {
  struct streamgrp *grp;  /* pointer to grp for next */
  int tab_idx;            /* current table index within grp */
  int count;              /* number of consumed table entries */
} streamtbe_iter_t;


extern streamset_t *StreamsetCreate(int init_cap2);
extern void StreamsetDestroy(streamset_t *set);
extern streamtbe_t *StreamsetAdd(streamset_t *set,
    struct stream *s, int *grp_idx);

extern void StreamsetEvent(streamset_t *set, streamtbe_t *tbe);
extern void StreamsetRemove(streamset_t *set, streamtbe_t *tbe);
extern void StreamsetReplace(streamset_t *set, streamtbe_t *tbe,
    struct stream *s);

extern void StreamsetChainStart(streamset_t *set);
extern void StreamsetChainAdd(streamset_t *set, int grp_idx);

extern void StreamsetIterateStart(streamset_t *set, streamtbe_iter_t *iter);
extern int  StreamsetIterateHasNext(streamset_t *set, streamtbe_iter_t *iter);
extern streamtbe_t *StreamsetIterateNext(streamset_t *set,
    streamtbe_iter_t *iter);

extern void StreamsetPrint(streamset_t *set, FILE *file);

#ifndef NDEBUG
extern void StreamsetDebug(streamset_t *set);
#endif


#endif /* _STREAMSET_H_ */
