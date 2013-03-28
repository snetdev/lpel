/*
 * Buffer, implemented as Single-Writer Single-Reader, unbounded
 * Concurrent accessed by two threads
 *
 */


#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "buffer.h"


typedef struct entry entry;
struct entry {
	void *data;
	entry *next;
};

struct buffer_t{
	entry *head;
	entry *tail;
	int count;
};

static entry *createEntry(void *data) {
  entry *e = (entry *) malloc(sizeof(entry));
  e->data = data;
  e->next = NULL;
  return e;
}


/**
 * Initialize a buffer.
 * Also allocates space for size void* items
 *
 * @param buf   pointer to buffer struct
 * @param size	unused (declared for same protocol with bounded buffer)
 */
buffer_t *LpelBufferInit(unsigned int size)
{
	buffer_t *buf = (buffer_t *) malloc(sizeof(buffer_t));
  buf->head = createEntry(NULL);
  buf->tail = buf->head;
  buf->count = 0;
  return buf;
}

/**
 * Cleanup the buffer.
 * Free the memory for the head
 *
 * @param buf   pointer to buffer struct
 */
void  LpelBufferCleanup(buffer_t *buf)
{
	 free(buf->head);
	 free(buf);
}


/**
 * Returns the top from a buffer
 *
 * @pre         no concurrent reads
 * @param buf   buffer to read from
 * @return      NULL if buffer is empty
 */
void *LpelBufferTop( buffer_t *buf)
{
	if (buf->head->next == NULL)
	    return NULL;
	  return buf->head->next->data;
}


/**
 * Consuming read from a stream
 *
 * Implementation note:
 * - modifies only pread pointer (not pwrite)
 *
 * @pre         no concurrent reads
 * @param buf   buffer to read from
 */
void LpelBufferPop( buffer_t *buf)
{
	if (buf->head->next == NULL)
	    return;
	  entry *t = buf->head;
	  buf->head = t->next;
	  buf->count--;
	  free(t);
}


/**
 * Check if there is space in the buffer
 *
 * @param buf   buffer to check
 * @pre         no concurrent calls
 */
int LpelBufferIsSpace( buffer_t *buf)
{
 //always return yes
	return 1;
}


/**
 * Put an item into the buffer
 *
 * Precondition: item != NULL
 *
 * Implementation note:
 * - modifies only pwrite pointer (not pread)
 *
 * @param buf   buffer to write to
 * @param item  data item (a pointer) to write
 * @pre         no concurrent writes
 * @pre         item != NULL
 * @pre         there has to be space in the buffer
 *              (check with BufferIsSpace)
 */
void LpelBufferPut( buffer_t *buf, void *item)
{
  assert( item != NULL );

  /* WRITE TO BUFFER */
  /* Write Memory Barrier: ensure all previous memory write
   * are visible to the other processors before any later
   * writes are executed.  This is an "expensive" memory fence
   * operation needed in all the architectures with a weak-ordering
   * memory model where stores can be executed out-or-order
   * (e.g. PowerPC). This is a no-op on Intel x86/x86-64 CPUs.
   */
  WMB();
  buf->tail->next = createEntry(item);
  buf->tail = buf->tail->next;
  buf->count++;
}

int LpelBufferCount(buffer_t *buf) {
	return buf->count;
}
