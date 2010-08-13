#ifndef _STREAMSET_H_
#define _STREAMSET_H

#include <stdio.h>


#define STREAMSET_GRP_SIZE  4


typedef struct streamset streamset_t;

typedef struct streamtbe streamtbe_t;

/* for convenience */
struct stream;

extern streamset_t *StreamsetCreate(int init_cap2);
extern void StreamsetDestroy(streamset_t *set);
extern streamtbe_t *StreamsetAdd(streamset_t *set,
    struct stream *s, int *grp_idx);

extern void StreamsetEvent(streamset_t *set, streamtbe_t *tbe);
extern void StreamsetRemove(streamset_t *set, streamtbe_t *tbe);
extern void StreamsetReplace(streamset_t *set, streamtbe_t *tbe, struct stream *s);
extern void StreamsetPrint(streamset_t *set, FILE *file);


#endif /* _STREAMSET_H_ */
