#include "http_server/thread_pool.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

typedef struct QueuedTask {
    ThreadPoolTaskFunction task_function;
    void*                  task_argument;
} QueuedTask;

struct ThreadPool {
    pthread_t*       worker_threads;
    size_t           worker_count;

    QueuedTask*      task_ring_buffer;
    size_t           task_queue_capacity;
    size_t           task_queue_size;
    size_t           task_queue_head;
    size_t           task_queue_tail;

    pthread_mutex_t  queue_mutex;
    pthread_cond_t   queue_not_empty_cond;
    pthread_cond_t   queue_not_full_cond;

    bool             shutdown_requested;
};

static void* worker_thread_main_loop(void* opaque_pool_pointer)
{
    ThreadPool* pool = (ThreadPool*)opaque_pool_pointer;

    for (;;) {
        pthread_mutex_lock(&pool->queue_mutex);
        while (!pool->shutdown_requested && pool->task_queue_size == 0) {
            pthread_cond_wait(&pool->queue_not_empty_cond, &pool->queue_mutex);
        }
        if (pool->shutdown_requested && pool->task_queue_size == 0) {
            pthread_mutex_unlock(&pool->queue_mutex);
            return NULL;
        }

        QueuedTask dequeued_task = pool->task_ring_buffer[pool->task_queue_head];
        pool->task_queue_head = (pool->task_queue_head + 1) % pool->task_queue_capacity;
        --pool->task_queue_size;

        pthread_cond_signal(&pool->queue_not_full_cond);
        pthread_mutex_unlock(&pool->queue_mutex);

        if (dequeued_task.task_function != NULL) {
            dequeued_task.task_function(dequeued_task.task_argument);
        }
    }
}

ThreadPool* thread_pool_create(size_t worker_count, size_t queue_capacity)
{
    if (worker_count == 0 || queue_capacity == 0) {
        return NULL;
    }

    ThreadPool* pool = (ThreadPool*)calloc(1, sizeof(ThreadPool));
    if (pool == NULL) return NULL;

    pool->worker_threads = (pthread_t*)calloc(worker_count, sizeof(pthread_t));
    pool->task_ring_buffer = (QueuedTask*)calloc(queue_capacity, sizeof(QueuedTask));
    if (pool->worker_threads == NULL || pool->task_ring_buffer == NULL) {
        free(pool->worker_threads);
        free(pool->task_ring_buffer);
        free(pool);
        return NULL;
    }

    pool->worker_count        = worker_count;
    pool->task_queue_capacity = queue_capacity;
    pool->task_queue_size     = 0;
    pool->task_queue_head     = 0;
    pool->task_queue_tail     = 0;
    pool->shutdown_requested  = false;

    if (pthread_mutex_init(&pool->queue_mutex, NULL) != 0
        || pthread_cond_init(&pool->queue_not_empty_cond, NULL) != 0
        || pthread_cond_init(&pool->queue_not_full_cond, NULL) != 0) {
        free(pool->worker_threads);
        free(pool->task_ring_buffer);
        free(pool);
        return NULL;
    }

    for (size_t worker_index = 0; worker_index < worker_count; ++worker_index) {
        if (pthread_create(&pool->worker_threads[worker_index], NULL,
                           worker_thread_main_loop, pool) != 0) {
            pool->worker_count = worker_index;
            thread_pool_shutdown_and_destroy(pool);
            return NULL;
        }
    }

    return pool;
}

bool thread_pool_submit_task(ThreadPool* pool,
                             ThreadPoolTaskFunction task_function,
                             void* task_argument)
{
    if (pool == NULL || task_function == NULL) return false;

    pthread_mutex_lock(&pool->queue_mutex);
    while (!pool->shutdown_requested
           && pool->task_queue_size == pool->task_queue_capacity) {
        pthread_cond_wait(&pool->queue_not_full_cond, &pool->queue_mutex);
    }
    if (pool->shutdown_requested) {
        pthread_mutex_unlock(&pool->queue_mutex);
        return false;
    }

    pool->task_ring_buffer[pool->task_queue_tail].task_function = task_function;
    pool->task_ring_buffer[pool->task_queue_tail].task_argument = task_argument;
    pool->task_queue_tail = (pool->task_queue_tail + 1) % pool->task_queue_capacity;
    ++pool->task_queue_size;

    pthread_cond_signal(&pool->queue_not_empty_cond);
    pthread_mutex_unlock(&pool->queue_mutex);
    return true;
}

void thread_pool_shutdown_and_destroy(ThreadPool* pool)
{
    if (pool == NULL) return;

    pthread_mutex_lock(&pool->queue_mutex);
    pool->shutdown_requested = true;
    pthread_cond_broadcast(&pool->queue_not_empty_cond);
    pthread_cond_broadcast(&pool->queue_not_full_cond);
    pthread_mutex_unlock(&pool->queue_mutex);

    for (size_t worker_index = 0; worker_index < pool->worker_count; ++worker_index) {
        pthread_join(pool->worker_threads[worker_index], NULL);
    }

    pthread_mutex_destroy(&pool->queue_mutex);
    pthread_cond_destroy(&pool->queue_not_empty_cond);
    pthread_cond_destroy(&pool->queue_not_full_cond);

    free(pool->worker_threads);
    free(pool->task_ring_buffer);
    free(pool);
}
