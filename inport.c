#include <stdlib.h>
#include <sched.h>

#include "inport.h"


inport_t *InportCreate(stream_t *s)
{
  inport_t *ip;

  ip = (inport_t *) malloc(sizeof(inport_t));
  ip->stream = s;
  s->cntwrite = &ip->cntwrite;

  return ip;
}

void InportWrite(inport_t *ip, void *item)
{
  while( !StreamIsSpace(NULL, ip->stream) ) {
    (void) sched_yield();
  }
  StreamWrite(NULL, ip->stream, item);
}

void InportDestroy(inport_t *ip)
{
  
  /* stream has to be freed by client */

  free(ip);
}
