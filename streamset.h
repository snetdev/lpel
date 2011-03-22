#ifndef _STREAMSET_H_
#define _STREAMSET_H_

#include "stream.h"

typedef lpel_stream_desc_t          *lpel_streamset_t;

typedef struct lpel_stream_iter_t    lpel_stream_iter_t;


void LpelStreamsetPut(lpel_streamset_t *set, lpel_stream_desc_t *node);
int  LpelStreamsetRemove(lpel_streamset_t *set, lpel_stream_desc_t *node);
int  LpelStreamsetIsEmpty(lpel_streamset_t *set);



lpel_stream_iter_t *LpelStreamIterCreate(lpel_streamset_t *set);
void LpelStreamIterDestroy(lpel_stream_iter_t *iter);

void LpelStreamIterReset(lpel_stream_iter_t *iter, lpel_streamset_t *set);
int  LpelStreamIterHasNext(lpel_stream_iter_t *iter);
lpel_stream_desc_t *LpelStreamIterNext(lpel_stream_iter_t *iter);
void LpelStreamIterAppend(lpel_stream_iter_t *iter, lpel_stream_desc_t *node);
void LpelStreamIterRemove(lpel_stream_iter_t *iter);


#endif /* _STREAMSET_H_ */
