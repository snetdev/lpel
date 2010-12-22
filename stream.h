#ifndef _STREAM_H_
#define _STREAM_H_


#include "bool.h"

#include "stream_desc.h"
#include "stream_list.h"

//#define STREAM_POLL_SPINLOCK

/* a stream */
typedef struct stream stream_t;



struct task;

stream_t *StreamCreate(void);
void StreamDestroy( stream_t *s);
stream_desc_t *StreamOpen( struct task *ct, stream_t *s, char mode);
void StreamClose( stream_desc_t *sd, bool destroy_s);
void StreamReplace( stream_desc_t *sd, stream_t *snew);
void *StreamPeek( stream_desc_t *sd);
void *StreamRead( stream_desc_t *sd);
void StreamWrite( stream_desc_t *sd, void *item);
void StreamPoll( stream_list_t *list);

int StreamResetDirty( struct task *t, void (*callback)(stream_desc_t *, void*), void *arg);




#endif /* _STREAM_H_ */
