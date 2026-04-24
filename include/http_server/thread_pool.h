#ifndef HTTP_SERVER_THREAD_POOL_H
#define HTTP_SERVER_THREAD_POOL_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file thread_pool.h
 * @brief Fixed-size worker pool with a bounded blocking task queue.
 *
 * The pool is task-agnostic: the caller supplies a callback and an opaque
 * argument when submitting work. Submission blocks while the queue is full and
 * unblocks once a worker dequeues a task.
 */

/**
 * @brief Opaque handle to a thread pool instance.
 */
typedef struct ThreadPool ThreadPool;

/**
 * @brief Function signature executed by a worker for each submitted task.
 *
 * @param task_argument Opaque pointer supplied to @ref thread_pool_submit_task.
 */
typedef void (*ThreadPoolTaskFunction)(void* task_argument);

/**
 * @brief Create a pool of worker threads.
 *
 * @param worker_count   Number of worker threads (must be >= 1).
 * @param queue_capacity Maximum number of in-flight tasks (must be >= 1).
 * @return Pointer to a heap-allocated pool on success, NULL on error.
 */
ThreadPool* thread_pool_create(size_t worker_count, size_t queue_capacity);

/**
 * @brief Submit a task to the pool. Blocks while the queue is full.
 *
 * @param pool          Target pool (must not be NULL).
 * @param task_function Callback invoked by the worker.
 * @param task_argument Opaque argument forwarded to @p task_function.
 * @return true on success, false when the pool has been shut down.
 */
bool thread_pool_submit_task(ThreadPool* pool,
                             ThreadPoolTaskFunction task_function,
                             void* task_argument);

/**
 * @brief Stop accepting tasks, drain the queue and join all workers.
 *
 * Safe to call multiple times; subsequent calls are no-ops.
 *
 * @param pool Pool to shut down (may be NULL).
 */
void thread_pool_shutdown_and_destroy(ThreadPool* pool);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_SERVER_THREAD_POOL_H */
