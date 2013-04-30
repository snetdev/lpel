#ifndef _BUFFER_H_
#define _BUFFER_H_


#include "arch/sysdep.h"

typedef struct buffer_t buffer_t;

buffer_t *LpelBufferInit(unsigned int size);
void  LpelBufferCleanup(buffer_t *buf);

void *LpelBufferTop(buffer_t *buf);
void  LpelBufferPop(buffer_t *buf);
int   LpelBufferIsSpace(buffer_t *buf);
void  LpelBufferPut(buffer_t *buf, void *item);
int	  LpelBufferCount(buffer_t *buf);
#endif /* _BUFFER_H_ */
