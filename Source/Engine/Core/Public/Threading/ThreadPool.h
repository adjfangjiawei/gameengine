
#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "Assert.h"
#include "Threading/ThreadPoolConfig.h"
#include "Threading/ThreadPoolStats.h"
#include "Threading/ThreadTask.h"
#include "CoreTypes.h"

namespace Engine {

    /**
     * @brief A thread pool for managing and executing concurrent tasks
     *
     * The ThreadPool maintains a pool of worker threads and a task queue.
     * Tasks can be submitted to the pool and will be executed by available
     * threads. The pool supports task priorities, cancellation, and statistics
     * tracking.
     */
    class ThreadPool {
      public:
        /**
         * @brief Get the singleton instance of the thread pool
         * @return Reference to the thread pool instance
         */
        static ThreadPool& Get();

        /**
         * @brief Initialize the thread pool with the specified configuration
         * @param config Configuration options for the thread pool
         */
        void Initialize(const ThreadPoolConfig& config = ThreadPoolConfig());

        /**
         * @brief Shutdown the thread pool and wait for all tasks to complete
         */
        void Shutdown();

        /**
         * @brief Submit a task to the thread pool with priority
         * @param task The task to execute
         * @param priority The priority level for the task
         * @return A future that will contain the result of the task
         */
        template <typename F, typename... Args>
        auto SubmitWithPriority(TaskPriority priority, F&& f, Args&&... args)
            -> std::future<std::invoke_result_t<F, Args...>> {  // Changed here
            using ReturnType = std::invoke_result_t<F, Args...>;  // And here

            auto task = std::make_shared<std::packaged_task<ReturnType()>>(
                [f = std::forward<F>(f),
                 args =
                     std::make_tuple(std::forward<Args>(args)...)]() mutable {
                    return std::apply(std::move(f), std::move(args));
                });

            std::future<ReturnType> result = task->get_future();
            auto submitTime = std::chrono::high_resolution_clock::now();

            auto threadTask = std::make_shared<ThreadTask>(
                [task, this, submitTime]() {
                    auto startTime = std::chrono::high_resolution_clock::now();
                    m_stats.AddWaitTime(
                        std::chrono::duration_cast<std::chrono::microseconds>(
                            startTime - submitTime));

                    (*task)();

                    auto endTime = std::chrono::high_resolution_clock::now();
                    m_stats.AddExecutionTime(
                        std::chrono::duration_cast<std::chrono::microseconds>(
                            endTime - startTime));
                },
                priority);

            {
                std::lock_guard<std::mutex> lock(m_queueMutex);
                ASSERT(!m_stopping, "Cannot submit task to stopped ThreadPool");

                if (m_config.maxQueueSize > 0 &&
                    m_tasks.size() >= m_config.maxQueueSize) {
                    throw std::runtime_error("Task queue is full");
                }

                m_tasks.push(threadTask);
                m_stats.UpdatePeakQueueSize(m_tasks.size());
                ++m_pendingTasks;
            }

            m_condition.notify_one();
            return result;
        }

        /**
         * @brief Submit a task to the thread pool with normal priority
         * @param task The task to execute
         * @return A future that will contain the result of the task
         */
        template <typename F, typename... Args>
        auto Submit(F&& f, Args&&... args)
            -> std::future<typename std::invoke_result<F, Args...>::type> {
            return SubmitWithPriority(TaskPriority::Normal,
                                      std::forward<F>(f),
                                      std::forward<Args>(args)...);
        }

        /**
         * @brief Get the number of tasks currently in the queue
         * @return Number of pending tasks
         */
        size_t GetPendingTaskCount() const;

        /**
         * @brief Get the number of worker threads in the pool
         * @return Number of worker threads
         */
        size_t GetThreadCount() const;

        /**
         * @brief Check if the thread pool is currently running
         * @return True if the pool is running
         */
        bool IsRunning() const { return !m_stopping; }

        /**
         * @brief Wait for all current tasks to complete
         */
        void WaitForCompletion();

        /**
         * @brief Get the current statistics of the thread pool
         * @return Reference to the thread pool statistics
         */
        const ThreadPoolStats& GetStats() const { return m_stats; }

        /**
         * @brief Reset the thread pool statistics
         */
        void ResetStats() { m_stats.Reset(); }

      private:
        ThreadPool() = default;
        ~ThreadPool();

        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;
        ThreadPool(ThreadPool&&) = delete;
        ThreadPool& operator=(ThreadPool&&) = delete;

        /**
         * @brief Worker thread function
         */
        void WorkerFunction();

      private:
        std::vector<std::thread> m_workers;
        std::priority_queue<ThreadTaskPtr,
                            std::vector<ThreadTaskPtr>,
                            ThreadTaskPtrCompare>
            m_tasks;

        mutable std::mutex m_queueMutex;
        std::condition_variable m_condition;
        std::atomic<bool> m_stopping{false};
        std::atomic<size_t> m_activeThreads{0};
        std::atomic<size_t> m_pendingTasks{0};

        ThreadPoolConfig m_config;
        ThreadPoolStats m_stats;
    };

    /**
     * @brief Helper class for managing task groups in the thread pool
     *
     * TaskGroup allows submitting multiple related tasks and waiting for
     * all of them to complete together.
     */
    class TaskGroup {
      public:
        TaskGroup() = default;
        ~TaskGroup() = default;

        /**
         * @brief Submit a task to the thread pool as part of this group
         * @param task The task to execute
         * @param priority The priority level for the task
         * @return A future that will contain the result of the task
         */
        template <typename F, typename... Args>
        auto SubmitTask(F&& f, Args&&... args)
            -> std::future<typename std::invoke_result<F, Args...>::type> {
            auto future = ThreadPool::Get().Submit(std::forward<F>(f),
                                                   std::forward<Args>(args)...);

            m_futures.push_back(
                std::make_shared<std::future<void>>(ThreadPool::Get().Submit(
                    [future = std::move(future)]() mutable { future.get(); })));
            return future;
        }

        /**
         * @brief Submit a task with priority to the thread pool as part of this
         * group
         * @param priority The priority level for the task
         * @param task The task to execute
         * @return A future that will contain the result of the task
         */
        template <typename F, typename... Args>
        auto SubmitTaskWithPriority(TaskPriority priority,
                                    F&& f,
                                    Args&&... args)
            -> std::future<typename std::invoke_result<F, Args...>::type> {
            auto future = ThreadPool::Get().SubmitWithPriority(
                priority, std::forward<F>(f), std::forward<Args>(args)...);

            m_futures.push_back(
                std::make_shared<std::future<void>>(ThreadPool::Get().Submit(
                    [future = std::move(future)]() mutable { future.get(); })));
            return future;
        }

        /**
         * @brief Wait for all tasks in this group to complete
         */
        void WaitForCompletion() {
            for (auto& future : m_futures) {
                future->get();
            }
            m_futures.clear();
        }

      private:
        std::vector<std::shared_ptr<std::future<void>>> m_futures;
    };

}  // namespace Engine
