#ifndef _ATOMICOP_H_
#define _ATOMICOP_H_


typedef struct { unsigned long val; } aulong_t;

#define aulong_read(x) ((x)->val)

#define aulong_inc(x) ((void)__sync_add_and_fetch(&(x)->val, 1))
#define aulong_dec(x) ((void)__sync_sub_and_fetch(&(x)->val, 1))

#define AULONG_INIT(x) {x}

#endif /* _ATOMICOP_H_ */
