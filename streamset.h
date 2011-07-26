#ifndef _STREAMSET_H_
#define _STREAMSET_H_

#include "lpel_name.h"
#include "stream.h"

typedef lpel_stream_desc_t          *lpel_streamset_t;

typedef struct lpel_stream_iter_t    lpel_stream_iter_t;


void LPEL_FUNC(StreamsetPut)(lpel_streamset_t *set, lpel_stream_desc_t *node);
int  LPEL_FUNC(StreamsetRemove)(lpel_streamset_t *set, lpel_stream_desc_t *node);
int  LPEL_FUNC(StreamsetIsEmpty)(lpel_streamset_t *set);



lpel_stream_iter_t *LPEL_FUNC(StreamIterCreate)(lpel_streamset_t *set);
void LPEL_FUNC(StreamIterDestroy)(lpel_stream_iter_t *iter);

void LPEL_FUNC(StreamIterReset)(lpel_stream_iter_t *iter, lpel_streamset_t *set);
int  LPEL_FUNC(StreamIterHasNext)(lpel_stream_iter_t *iter);
lpel_stream_desc_t *LPEL_FUNC(StreamIterNext)(lpel_stream_iter_t *iter);
void LPEL_FUNC(StreamIterAppend)(lpel_stream_iter_t *iter, lpel_stream_desc_t *node);
void LPEL_FUNC(StreamIterRemove)(lpel_stream_iter_t *iter);


#endif /* _STREAMSET_H_ */
