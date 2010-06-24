
#include <malloc.h>
#include "lpel_private.h"


struct streamarr {
  stream_t **array;
  unsigned int cnt; /* points to the next free index */
  unsigned int size;
};

/**
 * Allocate space of a stream array
 *
 * Precond: initsize > 0
 */
void StreamarrAlloc(streamarr_t *arr, const unsigned int initsize)
{
  arr->cnt = 0;
  arr->size = initsize;
  arr->array = (stream_t **)malloc( initsize*sizeof(stream_t *) );
}


void StreamarrFree(streamarr_t *arr)
{
  free( arr->array );
  arr->array = NULL;
  arr->size = 0;
  arr->cnt = 0;
}

/**
 * Add a stream to the stream array
 */
void StreamarrAdd(streamarr_t *arr, stream_t *s)
{
    if (arr->cnt >= arr->size) {
      arr->size *= 2;
      arr->array = (stream_t **)realloc( arr->array, arr->size );
      assert( arr->array != NULL );
    }
    arr->array[arr->cnt] = s;
    arr->cnt++;
}

/**
 * Iterate through the stream array,
 * calling func on each stream
 */
void StreamarrIter(streamarr_t *arr, void (*func)(stream_t *) )
{
  int i;
  for (i=0; i<arr->cnt; i++) {
    func( arr->array[i] );
  }
}

