#include <stdio.h>
#include <pthread.h>
#include <stdint.h>
#include "thread-pool.h"

void task(void *arg) {
    printf("Thread #%u working on %d\n", (int)pthread_self(), (int)arg);
}

int main() {

    puts("Making threadpool with 4 threads");
    thread_pool_t* thpool = tpool_init(4);

    puts("Adding 40 tasks to threadpool");
    int i;
    for (i = 0; i < 40; i++) 
        tpool_add_work(thpool, task, (void *)(uintptr_t)i);

    tpool_wait(thpool);
    puts("Killing threadpool");
    tpool_destroy(thpool);

    return 0;
}