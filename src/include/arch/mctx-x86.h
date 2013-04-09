


typedef void* mctx_t;

void ctx_init_internal(void**, void*, void(*)(void*), void*);
void ctx_swap_internal(void**, void**);


static inline int mctx_create(mctx_t *mctx, void *func, void *arg, char *sk_addr, long sk_size)
{
  ctx_init_internal(mctx, sk_addr+sk_size, func, arg);
  return 0;
}




static inline void mctx_switch(mctx_t *octx, mctx_t *nctx)
{
  ctx_swap_internal(octx, nctx);
}


