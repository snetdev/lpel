#ifndef _BUFFER_H_
#define _BUFFER_H_


#include "lpel_name.h"
#include "arch/sysdep.h"

/* 64bytes is the common size of a cache line */
#define longxCacheLine  (64/sizeof(long))

/* Padding is required to avoid false-sharing
   between core's private cache */
typedef struct {
  unsigned long pread;
//  volatile unsigned long pread;
  long padding1[longxCacheLine-1];
  unsigned long pwrite;
//  volatile unsigned long pwrite;
  long padding2[longxCacheLine-1];
  unsigned long size;
  void **data;
} buffer_t;


void  LPEL_FUNC(BufferInit)(buffer_t *buf, unsigned int size);
void  LPEL_FUNC(BufferCleanup)(buffer_t *buf);

void *LPEL_FUNC(BufferTop)(buffer_t *buf);
void  LPEL_FUNC(BufferPop)(buffer_t *buf);
int   LPEL_FUNC(BufferIsSpace)(buffer_t *buf);
void  LPEL_FUNC(BufferPut)(buffer_t *buf, void *item);

#endif /* _BUFFER_H_ */
