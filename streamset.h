#ifndef _STREAMSET_H_
#define _STREAMSET_H_

#include "lpel_name.h"
#include "stream.h"

typedef lpel_stream_desc_t          *lpel_streamset_t;

typedef struct lpel_stream_iter_t    lpel_stream_iter_t;


void LPEL_EXPORT(StreamsetPut)(lpel_streamset_t *set, lpel_stream_desc_t *node);
int  LPEL_EXPORT(StreamsetRemove)(lpel_streamset_t *set, lpel_stream_desc_t *node);
int  LPEL_EXPORT(StreamsetIsEmpty)(lpel_streamset_t *set);



lpel_stream_iter_t *LPEL_EXPORT(StreamIterCreate)(lpel_streamset_t *set);
void LPEL_EXPORT(StreamIterDestroy)(lpel_stream_iter_t *iter);

void LPEL_EXPORT(StreamIterReset)(lpel_stream_iter_t *iter, lpel_streamset_t *set);
int  LPEL_EXPORT(StreamIterHasNext)(lpel_stream_iter_t *iter);
lpel_stream_desc_t *LPEL_EXPORT(StreamIterNext)(lpel_stream_iter_t *iter);
void LPEL_EXPORT(StreamIterAppend)(lpel_stream_iter_t *iter, lpel_stream_desc_t *node);
void LPEL_EXPORT(StreamIterRemove)(lpel_stream_iter_t *iter);


#endif /* _STREAMSET_H_ */
