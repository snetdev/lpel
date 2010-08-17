/**
 * File:   set.c
 * Author: Daniel Prokesch
 *
 * Description:
 *
 * This file implements a very basic and simple set type,
 * that preferrably is used to store pointers.
 *
 * Its intended use is to keep a list of references, in
 * an array that can grow on demand automatically.
 * Hence, it is optimized for adding new elements.
 * It does not check for duplicates upon adding, and
 * upon removing, the first found reference is removed.
 *
 * Add: O(1)
 * Remove: O(n)
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include "set.h"


/**
 * Allocate space of a set
 *
 * Precond: initsize > 0
 */
void SetAlloc(set_t *s)
{
  s->cnt = 0;
  s->size = SET_INITSIZE;
  s->array = (void **)malloc( SET_INITSIZE*sizeof(void *) );
  if (s->array == NULL) {
    perror("SetAlloc() failed: could not alloc memory");
    exit(EXIT_FAILURE);
  }
}


void SetFree(set_t *s)
{
  free( s->array );
  s->array = NULL;
  s->size = 0;
  s->cnt = 0;
}

/**
 * Add an item to the set
 */
void SetAdd(set_t *s, void *item)
{
    /* item count reached size of set: increase size */
    if (s->cnt == s->size) {
      s->size += SET_DELTASIZE;
      s->array = (void **)realloc( s->array, s->size*sizeof(void *) );
      if (s->array == NULL) {
        perror("SetAdd() failed: could not realloc memory");
        exit(EXIT_FAILURE);
      }
    }
    s->array[s->cnt] = item;
    s->cnt++;
}

/**
 * Remove an item of the set
 */
bool SetRemove(set_t *s, void *item)
{
  int i, fnd;

  fnd = -1;
  for (i=0; i<s->cnt; i++) {
    if (s->array[i] == item) {
      fnd = i;
      break;
    }
  }
  
  /* if item not found in set, do nothing and return false*/
  if (fnd == -1) return false;
  
  /* remove item by decrementing cnt and placing
     item at cnt at the position of item to remove */
  s->cnt--;
  s->array[fnd] = s->array[s->cnt];
  

  /* possibly realloc smaller space generously */
  if (s->size - s->cnt > 2*SET_DELTASIZE) {
      s->size -= SET_DELTASIZE;
      s->array = (void **)realloc( s->array, s->size*sizeof(void *) );
      if (s->array == NULL) {
        perror("SetRemove() failed: could not realloc memory");
        exit(EXIT_FAILURE);
      }
  }

  return true;

}

/**
 * Apply func on each item
 */
void SetApply(set_t *s, void (*func)(void *) )
{
  int i;
  for (i=0; i<s->cnt; i++) {
    func( s->array[i] );
  }
}


/**
 * Get the size of the set, i.e.,
 * the number of items in the set
 */
unsigned int SetSize(set_t *s)
{
  return s->cnt;
}
