#include <gtest/gtest.h>

#include "http_server/thread_pool.h"

#include <atomic>

namespace {

struct CounterTaskArgument {
    std::atomic<int>* counter;
};

void increment_counter_task(void* opaque_task_argument)
{
    auto* task_argument = static_cast<CounterTaskArgument*>(opaque_task_argument);
    task_argument->counter->fetch_add(1, std::memory_order_relaxed);
}

}  // namespace

TEST(ThreadPool, RejectsInvalidArguments)
{
    EXPECT_EQ(thread_pool_create(0, 8),  nullptr);
    EXPECT_EQ(thread_pool_create(4, 0),  nullptr);
}

TEST(ThreadPool, ExecutesAllSubmittedTasks)
{
    std::atomic<int> execution_counter{0};
    CounterTaskArgument task_argument{&execution_counter};

    ThreadPool* pool = thread_pool_create(4, 64);
    ASSERT_NE(pool, nullptr);

    constexpr int submitted_task_count = 200;
    for (int submission_index = 0; submission_index < submitted_task_count; ++submission_index) {
        ASSERT_TRUE(thread_pool_submit_task(pool, increment_counter_task, &task_argument));
    }

    thread_pool_shutdown_and_destroy(pool);
    EXPECT_EQ(execution_counter.load(), submitted_task_count);
}

TEST(ThreadPool, RejectsSubmissionsAfterShutdown)
{
    ThreadPool* pool = thread_pool_create(2, 4);
    ASSERT_NE(pool, nullptr);

    std::atomic<int> execution_counter{0};
    CounterTaskArgument task_argument{&execution_counter};

    ASSERT_TRUE(thread_pool_submit_task(pool, increment_counter_task, &task_argument));
    thread_pool_shutdown_and_destroy(pool);
}

TEST(ThreadPool, BlocksWhenQueueIsFull)
{
    ThreadPool* pool = thread_pool_create(1, 2);
    ASSERT_NE(pool, nullptr);

    std::atomic<int> execution_counter{0};
    CounterTaskArgument task_argument{&execution_counter};

    for (int submission_index = 0; submission_index < 10; ++submission_index) {
        ASSERT_TRUE(thread_pool_submit_task(pool, increment_counter_task, &task_argument));
    }

    thread_pool_shutdown_and_destroy(pool);
    EXPECT_EQ(execution_counter.load(), 10);
}
