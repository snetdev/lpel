/*
 * Implements a streamtable as circular single linked list.
 *
 * Each task_t has a single streamtable where its streams opened
 * for reading and writing are maintained.
 */

#include <stdlib.h>
#include <assert.h>

#include "streamtable.h"


struct streamtbe {
  struct streamtbe *next;
  stream_t *s;
  char mode;
  int closed;
  unsigned long cnt;
};

/**
 * Add a new stream to the streamtable
 * 
 * Appends a stream in O(1).
 * As tab always points to the last element, after this operaton
 * tab points to the newly created entry.
 *
 * @return a pointer to the counter
 */
unsigned long *StreamtablePut(streamtable_t *tab, stream_t *s, char mode)
{
  struct streamtbe *node;
  
  node = (struct streamtbe *) malloc( sizeof(struct streamtbe) );
  node->s = s;
  node->mode = mode;
  node->closed = 0;
  node->cnt = 0;

  if (*tab  == NULL) {
    /* set is empty */
    *tab = node;
    node->next = node; /* selfloop */
  } else { 
    /* insert stream between last element=*tab and first element=(*tab)->next */
    node->next = (*tab)->next;
    (*tab)->next = node;
    *tab = node;
  }

  return &node->cnt;
}

/**
 * Marks a stream as closed in the streamtable
 *
 * The entry containing the stream is searched linearly, starting 
 * with entries added earlier in time.
 * In O(n).
 *
 * Precond: s is contained in the table
 */
void StreamtableMark(streamtable_t *tab, stream_t *s)
{
  /* cursor points initially to first element */
  struct streamtbe *cur = (*tab)->next;

  while (cur->s != s) {
    assert( cur != *tab ); /* not contained! */
    cur = cur->next;
  }

  /* now cur points to the entry containing s */
  cur->closed = 1;
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
