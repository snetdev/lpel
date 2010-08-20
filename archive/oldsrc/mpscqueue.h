#ifndef _MPSCQUEUE_H_
#define _MPSCQUEUE_H_



typedef struct mpscqueue mpscqueue_t;

extern mpscqueue_t *MPSCQueueCreate(void);
extern void MPSCQueueDestroy(mpscqueue_t *q);
extern void MPSCQueueEnqueue(mpscqueue_t *q, void *item);
extern void* MPSCQueueDequeue(mpscqueue_t *q);



#endif /* _MPSCQUEUE_H_ */
