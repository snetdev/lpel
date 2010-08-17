/**
 *
 */

#include <stdlib.h>
#include <assert.h>

#include "streamset.h"


#define STBE_OPEN     'O'
#define STBE_CLOSED   'C'
#define STBE_REPLACED 'R'
#define STBE_OBSOLETE 'X'


#define DIRTY_END   ((streamtbe_t *)-1)



/**
 * Mark a table entry as dirty
 */
static inline void MarkDirty(streamset_t *set, streamtbe_t *tbe)
{
  /* only add if not dirty yet */
  if (tbe->dirty == NULL) {
    /*
     * Set the dirty ptr of tbe to the dirty_list ptr of the set
     * and the dirty_list ptr of the set to tbe, i.e.,
     * append the tbe at the front of the dirty_list.
     * Initially, dirty_list of the set is empty DIRTY_END (!= NULL)
     */
    tbe->dirty = set->dirty_list;
    set->dirty_list = tbe;
  }
}





/**
 * Create a streamset
 *
 * @param init_cap2   initial capacity of the streamset.
 *                    streamset will fit 2^init_cap2 groups.
 * @pre               init_cap2 >= 0
 * @return            pointer to the created streamset
 */
streamset_t *StreamsetCreate(int init_cap2)
{
  streamset_t *set;

  set = (streamset_t *) malloc( sizeof(streamset_t) );
  set->grp_capacity = (1<<init_cap2);
  /* allocate lookup table, initialized to NULL values */
  set->lookup = (struct streamgrp **) calloc(
      set->grp_capacity, sizeof(struct streamgrp *)
      );
  /* point to the first position */
  set->idx_grp = 0;
  set->idx_tab = 0;
  /* there is no obsolete entry initially */
  set->cnt_obsolete = 0;
  /* dirty list is empty */
  set->dirty_list = DIRTY_END;

  /* group chain is uninitialized */
  
  return set;
}


/**
 * Destroy a streamset
 *
 * @param set   streamset to destroy
 */
void StreamsetDestroy(streamset_t *set)
{
  int i;
  struct streamgrp *grp;

  /* iterate through lookup table, free groups */
  for (i=0; i<set->grp_capacity; i++) {
    grp = set->lookup[i];
    if (grp != NULL) free(grp);
  }
  /* free the lookup table itself */
  free(set->lookup);

  /* free the set itself */
  free(set);
}





/**
 * Add a stream to the streamset
 *
 * @param set     set in which to add a stream
 * @param s       stream to add
 * @param grp_idx OUT param; if grp_idx != NULL, group index
 *                will be written to the given location
 * @return        pointer to the stream table entry
 */
streamtbe_t *StreamsetAdd(streamset_t *set, struct stream *s, int *grp_idx)
{
  struct streamgrp *grp;
  streamtbe_t *ste;
  int ret_idx;

  /* first try to obtain an obsolete entry */
  if (set->cnt_obsolete > 0) {
    int i,j;
    ste = NULL;
    /* iterate through all table entries from the beginning */
    for(i=0; i<=set->idx_grp; i++) {
      grp = set->lookup[i];
      for (j=0; j<STREAMSET_GRP_SIZE; j++) {
        if (grp->tab[j].state == STBE_OBSOLETE) {
          ste = &grp->tab[j];
          ret_idx = i;
          goto found_obsolete;
        }
      }
    }
    found_obsolete:
    /* one will be found eventually (lower position than idx_grp/idx_tab) */
    assert(ste != NULL);
    set->cnt_obsolete--;
  
  } else {
    /*
     * No obsolete entry could be reused. Get a new entry.
     * Assume that set->idx_grp and set->idx_tab refer to
     * the next free position, but it has not to exist yet
     */

    /* check if the current capacity is sufficient */
    if (set->idx_grp == set->grp_capacity) {
      /* double capacity and resize lookup table*/
      struct streamgrp **tmp;
      int i;

      set->grp_capacity *= 2;
      tmp = (struct streamgrp **) realloc(
          set->lookup, set->grp_capacity * sizeof(struct streamgrp *)
          );

      if (tmp == NULL) {
        /*TODO fatal error */
      }
      set->lookup = tmp;

      /* initialize newly allocated mem to NULL */
      for (i=set->idx_grp; i<set->grp_capacity; i++) {
        set->lookup[i] = NULL;
      }
    }


    /* if the grp does not exist yet, we have to create a new one */
    if (set->lookup[set->idx_grp] == NULL) {
      /* if that is the case, idx_tab must be 0 as well */
      assert( set->idx_tab == 0 );

      grp  = (struct streamgrp *) malloc( sizeof(struct streamgrp) );
      grp->next = NULL;
      /* put in lookup table */
      set->lookup[set->idx_grp] = grp;
    } else {
      grp = set->lookup[set->idx_grp];
    }
    /* get a ptr to the table entry */
    ste = &grp->tab[set->idx_tab];
    ret_idx = set->idx_grp;

    /* increment the next-free position */
    set->idx_tab++;
    if (set->idx_tab == STREAMSET_GRP_SIZE) {
      /* wrap */
      set->idx_tab = 0;
      set->idx_grp++;
    }
  }
  /* ste points to the right table entry */
  /* ret_idx contains the group number of ste */

  /* set out parameter grp_idx */
  if (grp_idx != NULL) *grp_idx = ret_idx;
  
  /* fill the entry */
  ste->s = s;
  ste->state = STBE_OPEN;
  ste->cnt = 0;

  /* mark the entry as dirty */
  MarkDirty(set, ste);

  /* return the pointer to the table entry */
  return ste;
}


/**
 * Signal a state change for the stream
 *
 * Increments the counter of teh table entry and marks it dirty
 */
void StreamsetEvent(streamset_t *set, streamtbe_t *tbe)
{
  assert( tbe->state == STBE_OPEN || tbe->state == STBE_REPLACED );

  tbe->cnt++;
  MarkDirty(set, tbe);
}


/**
 * Remove a table entry
 *
 * It remains in the set as closed and marked dirty,
 * until the next call to StreamsetPrint
 */
void StreamsetRemove(streamset_t *set, streamtbe_t *tbe)
{
  assert( tbe->state == STBE_OPEN || tbe->state == STBE_REPLACED );

  tbe->state = STBE_CLOSED;
  MarkDirty(set, tbe);
}

/**
 * Replace a stream in the stream table entry
 *
 * Resets counter, marks it as replaced and dirty
 */
void StreamsetReplace(streamset_t *set, streamtbe_t *tbe, struct stream *s)
{
  assert( tbe->state == STBE_OPEN || tbe->state == STBE_REPLACED );

  tbe->s = s;
  tbe->cnt = 0;
  tbe->state = STBE_REPLACED;
  MarkDirty(set, tbe);
}



/**
 * Start collecting groups in the chain
 *
 * @note  changes only hnd and count of set->chain
 */
void StreamsetChainStart(streamset_t *set)
{
  set->chain.hnd = NULL;
  set->chain.count = 0;
}

/**
 * Add to the chain of groups
 *
 * The groups are collected in a circular singly linked list.
 * The chain handle always points to the last group in the list.
 *
 * @note  changes only hnd and count of set->chain
 * @pre   grp_idx is only added once, the grp with grp_idx exists
 */
void StreamsetChainAdd(streamset_t *set, int grp_idx)
{
  struct streamgrp *grp;

  grp = set->lookup[grp_idx];
  assert( grp != NULL );

  if (set->chain.hnd == NULL) {
    /* chain is empty */
    set->chain.hnd = grp;
    grp->next = grp; /* self-loop */
  } else {
    /* insert grp between last (=set->chain)
       and first (=set->chain->next) */
    grp->next = set->chain.hnd->next;
    set->chain.hnd->next = grp;
    /* chain always points to last grp */
    set->chain.hnd = grp;
  }
  
  set->chain.count += ( set->idx_grp==grp_idx && set->idx_tab!=0 )
    ? set->idx_grp : STREAMSET_GRP_SIZE;
}



/**
 *
 */
void StreamsetIterateStart(streamset_t *set, streamtbe_iter_t *iter)
{
  iter->grp = NULL;
  iter->tab_idx = 0;
  iter->count = 0;
  if (set->chain.hnd != NULL) {
    iter->grp = set->chain.hnd->next;
  }
}


/**
 * @return the number of entries left to consume
 */
int StreamsetIterateHasNext(streamset_t *set, streamtbe_iter_t *iter)
{
  return set->chain.count - iter->count;
}


/**
 * get the next tbe in the chain
 */
streamtbe_t *StreamsetIterateNext(streamset_t *set, streamtbe_iter_t *iter)
{
  streamtbe_t *next;

  next = &iter->grp->tab[ iter->tab_idx ];
  iter->count++;

  /* advance next */
  iter->tab_idx++;
  if (iter->tab_idx == STREAMSET_GRP_SIZE) {
    iter->tab_idx = 0;
    iter->grp = iter->grp->next;
  }
  return next;
}




/**
 * Prints information about the dirty table entries
 *
 * @param set   the set for which the information is to be printed
 * @param file  the file where to print the information to, or NULL
 *              if only the dirty list should be cleared
 * @pre         if file != NULL, it must be open for writing
 */
void StreamsetPrint(streamset_t *set, FILE *file)
{
  streamtbe_t *tbe, *next;

  /* Iterate through dirty list */
  tbe = set->dirty_list;
  
  if (file!=NULL) {
    fprintf( file,"[" );
  }

  while (tbe != DIRTY_END) {
    /* print tbe */
    if (file!=NULL) {
      fprintf( file,
          "%p,%c,%lu;",
          tbe->s, tbe->state, tbe->cnt
          );
    }
    /* update states */
    if (tbe->state == STBE_REPLACED) { tbe->state = STBE_OPEN; }
    if (tbe->state == STBE_CLOSED) {
      tbe->state = STBE_OBSOLETE;
      set->cnt_obsolete++;
    }
    /* get the next dirty entry, and clear the link in the current entry */
    next = tbe->dirty;
    tbe->dirty = NULL;
    tbe = next;
  }
  if (file!=NULL) {
    fprintf( file,"]" );
  }
  /* dirty_list is now empty again */
  set->dirty_list = DIRTY_END;
}

#ifndef NDEBUG
/**
 * Print complete state of streamset for debugging purposes
 */
void StreamsetDebug(streamset_t *set)
{
  int i,j;
  streamtbe_t *tbe;
  struct streamgrp *grp;

  fprintf(stderr, "\n=== Streamset state (debug) ============\n");

  fprintf(stderr,
    "Group capacity %d\n"
    "idx_grp %d, idx_tab %d\n"
    "dirty_list %p\n",
    set->grp_capacity,
    set->idx_grp, set->idx_tab,
    set->dirty_list
    );

  fprintf(stderr,"\nLookup:\n");
  for (i=0; i<set->grp_capacity; i++) {
    fprintf(stderr,"[%d] %p\n",i, set->lookup[i]);
  }
  
  fprintf(stderr,"\nDirty list state:\n");
  tbe = set->dirty_list;
  while (tbe != DIRTY_END) {
    fprintf(stderr, "-> [%p]", tbe);
    tbe = tbe->dirty;
  }
  fprintf(stderr, "-> [%p]\n", tbe);

  for (i=0; i<set->grp_capacity; i++) {
    grp = set->lookup[i];
    if (grp != NULL) {
      fprintf(stderr,"\nGroup [%d: %p]\n",i, set->lookup[i]);
      for (j=0; j<STREAMSET_GRP_SIZE; j++) {
        tbe = &grp->tab[j];
        if (j>=set->idx_tab && i>=set->idx_grp) {
          fprintf(stderr, "[%d: %p]\n", j, tbe);
          continue;
        }
        fprintf(stderr, "[%d: %p] stream %p  state %c  cnt %lu  dirty %p\n",
            j, tbe, tbe->s, tbe->state, tbe->cnt, tbe->dirty
            );
      }
    }
  }

  fprintf(stderr, "\n========================================\n");
}
#endif
