

#include <pthread.h>

#define PTHR_STREAM_BUFFER_SIZE 16



typedef struct pthr_stream_t pthr_stream_t;

typedef struct pthr_stream_desc_t pthr_stream_desc_t;

typedef struct pthr_stream_desc_t *pthr_stream_set_t;


pthr_stream_t *PthrStreamCreate(void);
void PthrStreamDestroy(pthr_stream_t *s);

pthr_stream_desc_t *PthrStreamOpen(pthr_stream_t *stream, char mode);
void PthrStreamClose(pthr_stream_desc_t *sd, int destroy_s);

void PthrStreamWrite(pthr_stream_desc_t *s, void *item);
void *PthrStreamRead(pthr_stream_desc_t *s);

pthr_stream_desc_t *PthrStreamPoll(pthr_stream_set_t *set);
