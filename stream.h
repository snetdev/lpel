#ifndef _STREAM_H_
#define _STREAM_H_


typedef struct stream stream_t;



extern stream_t *StreamCreate(void);
extern void StreamDestroy(stream_t *s);
extern void *StreamPeek(stream_t *s);
extern void *StreamRead(stream_t *s);
extern bool StreamIsSpace(stream_t *s);
extern void StreamWrite(stream_t *s, void *item);


#endif /* _STREAM_H_ */
