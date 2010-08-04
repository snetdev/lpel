#ifndef _STREAMTABLE_H_
#define _STREAMTABLE_H_

#include <stdio.h>

#include "bool.h"


typedef struct streamtbe *streamtable_t;

struct stream;

extern unsigned long *StreamtablePut(streamtable_t *tab,
                        struct stream *s, char mode);
extern void StreamtableMark(streamtable_t *tab, struct stream *s);
extern unsigned int StreamtableClean(streamtable_t *tab);
extern bool StreamtableEmpty(streamtable_t *tab);
extern void StreamtablePrint(streamtable_t *tab, FILE *file);


#endif /* _STREAMTABLE_H_ */
