#ifndef _SET_H_
#define _SET_H_

#include "bool.h"

#define SET_INITSIZE  16
#define SET_DELTASIZE 16

typedef struct {
  void **array;
  unsigned int cnt; /* points to the next free index */
  unsigned int size;
} set_t;



extern void SetAlloc(set_t *s);
extern void SetFree(set_t *s);
extern void SetAdd(set_t *s, void *item);
extern bool SetRemove(set_t *s, void *item);
extern void SetApply(set_t *s, void (*func)(void *) );



#endif /* _SET_H_ */
