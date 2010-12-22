#ifndef _STREAM_LIST_H_
#define _STREAM_LIST_H_

#include "stream_desc.h"

/** a handle to a stream descriptor list */
typedef stream_desc_t *stream_list_t;

/** an iterator for a stream descriptor list */
typedef struct stream_iter stream_iter_t;



void StreamListAppend( stream_list_t *lst, stream_desc_t *node);
int StreamListIsEmpty( stream_list_t *lst);

stream_iter_t *StreamIterCreate( stream_list_t *lst);
void StreamIterDestroy( stream_iter_t *iter);
void StreamIterReset( stream_list_t *lst, stream_iter_t *iter);
int StreamIterHasNext( stream_iter_t *iter);
stream_desc_t *StreamIterNext( stream_iter_t *iter);
void StreamIterAppend( stream_iter_t *iter, stream_desc_t *node);
void StreamIterRemove( stream_iter_t *iter);


#endif /* _STREAM_LIST_H_ */
