// simple thread pool library
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include  <sys/prctl.h>

#include "thread-pool.h"

#define max(a, b) (a) >= (b) ? (a) : (b)

volatile int thread_keepalive;

typedef struct bsemaphor_t {
    pthread_mutex_t lock;
    pthread_cond_t cond;
    int v;
} bsemaphor_t;

static bsemaphor_t* bsem_init(int val) {
    if (val < 0 || val > 1) {
        // TODO report some errors
        exit(1);
    }

    bsemaphor_t* bsem = (bsemaphor_t*) malloc(sizeof(bsemaphor_t));
    pthread_mutex_init(&(bsem->lock), NULL);
    pthread_cond_init(&(bsem->cond), NULL);
    bsem->v = val;
    return bsem;
}

static void bsem_reset(bsemaphor_t** bsem) {
    *bsem = bsem_init(0);
}

static void bsem_post(bsemaphor_t* bsem) {
    pthread_mutex_lock(&(bsem->lock));
    bsem->v = 1;
    pthread_cond_signal(&(bsem->cond));
    pthread_mutex_unlock(&(bsem->lock));
}

static void bsem_post_all(bsemaphor_t* bsem) {
    pthread_mutex_lock(&(bsem->lock));
    bsem->v = 1;
    pthread_cond_broadcast(&(bsem->cond));
    pthread_mutex_unlock(&(bsem->lock));
}

static void bsem_wait(bsemaphor_t* bsem) {
    pthread_mutex_lock(&(bsem->lock));
    while(bsem->v != 1)
        pthread_cond_wait(&(bsem->cond), &(bsem->lock));
    bsem->v = 0;
    pthread_mutex_unlock(&(bsem->lock));
}

typedef struct job_t {
    struct job_t* next;
    action_t action;
    void* args;
} job_t;

typedef struct queue_t {
    pthread_mutex_t lock;
    job_t* front;
    job_t* rear;
    int len;
    bsemaphor_t* has_jobs;
} queue_t;

static queue_t* queue_init() {
    queue_t* queue = (queue_t*) malloc(sizeof(queue_t));
    queue->front = NULL;
    queue->rear = NULL;
    queue->len = 0;

    pthread_mutex_init(&(queue->lock), NULL);
    queue->has_jobs = bsem_init(0);
    return queue;
}

static job_t* queue_pull(queue_t* queue) {
    pthread_mutex_lock(&(queue->lock));
    job_t* job = queue->front;

    switch (queue->len) {
        case 0: break;
            // TODO maybe block untile there is a job
        case 1:
            queue->front = queue->rear = NULL;
            queue->len = 0;
            break;
        default:
            job_t *job = queue->front;
            queue->front = job->next;
            queue->len --;
            bsem_post(queue->has_jobs);
    }

    pthread_mutex_unlock(&(queue->lock));
    return job;
}

static void queue_push(queue_t* queue, job_t* job) {
    pthread_mutex_lock(&(queue->lock));
    switch(queue->len) {
        case 0:
            queue->front = queue->rear = job;
            break;
        default:
            queue->rear->next = job;
            queue->rear = job;
    }
    queue->len += 1;
    bsem_post(queue->has_jobs);
    pthread_mutex_unlock(&(queue->lock));
}

static void queue_clear(queue_t* queue) {
    while(queue->len) {
        free(queue_pull(queue));
    }

    queue->front = NULL;
    queue->rear = NULL;
    queue->len = 0;
}

static void queue_destroy(queue_t* queue) {
    queue_clear(queue);
    free(queue);
}

typedef struct thread {
    int id;
    pthread_t pthread;
    struct _thread_pool *pool;
} thread_t;

typedef struct _thread_pool {
    thread_t** threads;
    volatile int alive_thread_count;
    volatile int working_thread_count;
    pthread_mutex_t thcount_lock;
    pthread_cond_t all_idel;
    queue_t* queue;
} _thread_pool;


static void thread_hold(int sig_id) {
    (void) sig_id;
    // threads_on_hold += 1;
    // while(thread_on_hold) {
    //     sleep(1);
    // }
}

static int thread_do(thread_t* thread) {
    char name[16] = {0};
    sprintf(name, "thread-%d", thread->id);
    
    #if defined(__linux)
    prctl(PR_SET_NAME, name);
    #endif
    
    struct sigaction sig_act;
    sigemptyset(&sig_act.sa_mask);
    sig_act.sa_flags = 0;
    sig_act.sa_handler = thread_hold;
    if(sigaction(SIGUSR1, &sig_act, NULL) == -1) {
        // TODO report an error
    }

    _thread_pool* pool = thread->pool;
    pthread_mutex_lock(&(pool->thcount_lock));
    pool->alive_thread_count++;
    pthread_mutex_unlock(&(pool->thcount_lock));

    while(thread_keepalive) {
        bsem_wait(pool->queue->has_jobs);
        if(thread_keepalive) {
            pthread_mutex_lock(&(pool->thcount_lock));
            pool->working_thread_count++;
            pthread_mutex_unlock(&(pool->thcount_lock));
            job_t* job = queue_pull(pool->queue);
            if(job) {
                job->action(job->args);
                free(job);
            }
            pthread_mutex_lock(&(pool->thcount_lock));
            pool->working_thread_count--;
            // printf("%d %d\n", pool->working_thread_count, pool->alive_thread_count);
            if(pool->working_thread_count == 0)
                pthread_cond_signal(&(pool->all_idel));
            pthread_mutex_unlock(&(thread->pool->thcount_lock));
        }
    }

    pthread_mutex_lock(&(thread->pool->thcount_lock));
    thread->pool->alive_thread_count--;
    pthread_mutex_unlock(&(thread->pool->thcount_lock));
    return NULL;
}

static int thread_init(_thread_pool* pool, thread_t** thread, int id) {
    
    *thread = (thread_t*) malloc(sizeof(thread_t));
    if(*thread == NULL ) {
        // print some error
        return -1;
    }
    (*thread)->pool = pool;
    (*thread)->id = id;

    pthread_create(
        &((*thread)->pthread), 
        NULL, 
        (void * (*) (void *)) thread_do, 
        (*thread)
    );
    pthread_detach((*thread)->pthread);
    return 0;
}


_thread_pool* tpool_init(int num){
    thread_keepalive = 1;
    num = max(num, 0);

    _thread_pool* pool = (_thread_pool*) malloc(sizeof(struct _thread_pool));
    pool->alive_thread_count = 0;
    pool->working_thread_count = 0;

    pool->threads = (thread_t**) malloc(num * sizeof(thread_t*));
    pool->queue = queue_init();

    pthread_mutex_init(&(pool->thcount_lock), NULL);
    pthread_cond_init(&(pool->all_idel), NULL);
    for(int i=0; i<num; i++)
        thread_init(pool, &pool->threads[i], i);
    
    while(!pool->alive_thread_count);
    return pool;
}

int tpool_add_work(_thread_pool* pool, action_t action, void* args) {
    job_t* job = (job_t*) malloc(sizeof(job_t));
    if (job == NULL) {
        return -1;
    }
    job->action = action;
    job->args = args;
    queue_push(pool->queue, job);
    return 0;
}

void tpool_wait(thread_pool_t *pool) {
    pthread_mutex_lock(&(pool->thcount_lock));
    while (pool->working_thread_count || pool->queue->len)
        pthread_cond_wait(&(pool->all_idel), &(pool->thcount_lock));
    pthread_mutex_unlock(&(pool->thcount_lock));
}

void tpool_destroy(thread_pool_t *pool) {
    if (pool == NULL) return;
}

void tpool_pause(thread_pool_t *pool) {

}

void tpool_resume(thread_pool_t *tpool) {

}

int tpool_woriking_count(thread_pool_t *pool) {
    return pool->working_thread_count;
}
