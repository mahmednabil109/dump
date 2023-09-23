#ifndef M_THREAD_POOL
#define M_THREAD_POOL

typedef void (*action_t)(void*);
typedef struct _thread_pool thread_pool_t;

thread_pool_t* tpool_init(int num);
int tpool_add_work(thread_pool_t* tpool, action_t action, void *args);
void tpool_wait(thread_pool_t* tpool);
void tpool_pause(thread_pool_t* tpool);
void tpool_resume(thread_pool_t *tpool);
void tpool_destroy(thread_pool_t *tpool);
int tpool_woriking_count(thread_pool_t* tpool);

#endif