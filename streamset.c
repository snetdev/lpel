/**
 * Implements a streamtable as circular single linked list.
 *
 * Each task_t has a single streamtable where its streams opened
 * for reading and writing are maintained.
 */

#include <stdlib.h>
#include <assert.h>

//#include "streamset.h"

#define STREAMSET_GRP_SIZE  4


typedef struct streamset streamset_t;


/* for convenience */
struct stream;
typedef struct stream stream_t;


typedef struct streamtbe streamtbe_t;


typedef enum {
  OPEN=0, CLOSED=1, OBSOLETE=2
} streamtbe_state_t;

/**
 * each stream has a stream table entry
 * - a pointer to this should be placed in a (hash)set for "dirty" streams
 */
struct streamtbe {
  stream_t *s;
  streamtbe_state_t state;
  unsigned long cnt;
};

struct streamgrp {
  int grp_idx;
  struct streamgrp *next;
  struct streamgrp *it_next;  /* for the iterator */
  struct streamtbe tab[STREAMSET_GRP_SIZE];
};


struct streamset {
  int grp_capacity;
  struct streamgrp *lookup[]; /* array of pointers to the grps */
  int idx_grp; /* current grp for next free entry */
  int idx_tab; /* current index of the tab within
                  the grp for next free entry */
  int cnt_obsolete; /* number of obsolete stream table entries */
};




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
  struct streamgrp *first;

  set = (streamset_t *) malloc( sizeof(streamset) );
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
 * @param grp_idx TODO OUT param
 *                group index will be written to the given location
 * @return        pointer to the stream table entry
 */
streamtbe_t *StreamsetAdd(streamset_t *set, stream_t *s, int *grp_idx)
{
  struct streamgrp *grp;
  streamtbe_t *ste;


  /* first try to obtain an obsolete entry */
  if (set->cnt_obsolete > 0) {
    /* iterate through all table entries from the beginning */
    /* one will be found eventually (lower position than idx_grp/idx_tab) */
    //TODO
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
      grp->grp_idx = set->idx_grp;
      grp->next = NULL;
      grp->it_next = NULL;
      /* put in lookup table */
      set->lookup[set->idx_grp] = grp;
    } else {
      grp = set->lookup[set->idx_grp];
    }
  }
  /* grp points to the right group */

  /* set out parameter grp_idx */
  *grp_idx = grp->grp_idx;
  
  /* get a ptr to the table entry */
  ste = &grp->tab[set->idx_tab];
  /* fill the entry */
  ste->s = s;
  ste->state = OPEN;
  ste->cnt = 0;

  /* increment the next-free position */
  set->idx_tab++;
  if (set->idx_tab == STREAMSET_GRP_SIZE) {
    /* wrap */
    set->idx_tab = 0;
    set->idx_grp++;
  }

  /* return the pointer to the table entry */
  return ste;
}




/**
 * Marks a stream as closed in the streamtable
 *
 * The entry containing the stream is searched linearly, starting 
 * with entries added earlier in time.
 * In O(n).
 *
 * @pre    s is contained in the table
 * @return the mode in which s was opened
 */
char StreamtableMark(streamtable_t *tab, stream_t *s)
{
  /* cursor points initially to first element */
  struct streamtbe *cur = (*tab)->next;

  while (cur->s != s) {
    assert( cur != *tab ); /* not contained! */
    cur = cur->next;
  }

  /* now cur points to the entry containing s */
  cur->closed = 1;
  return cur->mode;
}

/**
 * Clean the table, i.e., remove all entries containing streams
 * marked as closed.
 *
 * In O(n).
 */
unsigned int StreamtableClean(streamtable_t *tab)
{
  struct streamtbe *prev, *cur;
  unsigned int cnt;
  
  /* if the table is empty, there is nothing to do */
  if (*tab == NULL) return 0;

  cnt = 0;

  prev = *tab;
  do {
    cur = prev->next;

    /* hande case if there is only a single element */
    if (prev == cur) {
      if (cur->closed != 0) {
        cnt++;
        free(cur);
        *tab = NULL;
        break;
      }
    } else {

      if (cur->closed != 0) {
        /* if the last element is to be deleted, correct *tab */
        if (*tab == cur) *tab = prev;
        /* remove cur */
        prev->next = cur->next;
        cnt++;
        free(cur); /* cur becomes invalid */
        cur = NULL;
      } else {
        /* advance prev  */
        prev = cur;
      }
    }
    /* we must check on cur here as well: in case the first element was
       removed, prev == *tab still */
  } while ( prev != (*tab) || cur == NULL);

  return cnt;
}


/**
 * 
 */
bool StreamtableEmpty(streamtable_t *tab)
{
  return (*tab == NULL);
}

/**
 * Prints the streamtable to a file
 *
 * Precond: file is opened for writing
 */
void StreamtablePrint(streamtable_t *tab, FILE *file)
{
  struct streamtbe *cur;

  /* fprintf( file,"--TAB--\n" ); */
  fprintf( file,"[" );

  if (*tab != NULL) {
    cur = (*tab)->next;
    do {
    
      fprintf( file,
        /* "%p %c %d %lu\n", */
        "%p,%c,%d,%lu;",
        cur->s, cur->mode, cur->closed, cur->cnt
        );
      cur = cur->next;
    } while (cur != (*tab)->next);
  }

  /* fprintf( file,"-------\n" ); */
  fprintf( file,"]" );

}
