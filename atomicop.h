#ifndef _ATOMICOP_H_
#define _ATOMICOP_H_


typedef struct { unsigned long val; } aulong_t;

#define aulong_read(x) ((x)->val)

#define aulong_inc(x) ((void)__sync_add_and_fetch(&(x)->val, 1))
#define aulong_dec(x) ((void)__sync_sub_and_fetch(&(x)->val, 1))
#define AULONG_INIT(x) {x}


typedef aulong_t sequencer_t;
#define ticket(s) (__sync_fetch_and_add(&(s)->val,1))
#define SEQUENCER_INIT  AULONG_INIT(0)

#endif /* _ATOMICOP_H_ */
