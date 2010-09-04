#include <stdlib.h>
#include <sched.h>

#include "outport.h"


outport_t *OutportCreate(stream_t *s)
{
  outport_t *op;

  op = (outport_t *) malloc(sizeof(outport_t));
  op->stream = s;

  return op;
}

void *OutportRead(outport_t *op)
{
  while( StreamPeek(NULL, op->stream) == NULL ) {
    (void) sched_yield();
  }
  return StreamRead(NULL, op->stream);
}

void OutportDestroy(outport_t *op)
{
  
  /* stream has to be freed by client */

  free(op);
}
