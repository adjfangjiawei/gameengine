
#include "Core/Threading/ThreadPool.h"

#include "Core/Assert.h"
#include "Core/Log/LogSystem.h"

namespace Engine {

    ThreadPool& ThreadPool::Get() {
        static ThreadPool instance;
        return instance;
    }

    void ThreadPool::Initialize(const ThreadPoolConfig& config) {
        ASSERT(!IsRunning(), "Thread pool is already initialized");

        m_config = config;
        size_t threadCount = m_config.threadCount;

        if (threadCount == 0) {
            threadCount = std::thread::hardware_concurrency();
            // Ensure at least one thread
            if (threadCount == 0) {
                threadCount = 1;
            }
        }

        m_stopping = false;
        m_workers.reserve(threadCount);

        try {
            // Create worker threads
            for (size_t i = 0; i < threadCount; ++i) {
                m_workers.emplace_back(&ThreadPool::WorkerFunction, this);

                // Set thread name if supported by platform
                if (m_config.threadNamePrefix) {
                    // Note: Platform-specific thread naming would go here
                    // This is just a placeholder as thread naming is
                    // platform-dependent
                }
            }

            LOG_INFO("Thread pool initialized with {} threads", threadCount);
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to initialize thread pool: {}", e.what());
            Shutdown();
            throw;
        }
    }

    void ThreadPool::Shutdown() {
        if (!IsRunning()) {
            return;
        }

        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            m_stopping = true;
        }

        // Wake up all worker threads
        m_condition.notify_all();

        // Wait for all threads to complete with timeout
        if (m_config.shutdownTimeoutMs > 0) {
            auto endTime =
                std::chrono::steady_clock::now() +
                std::chrono::milliseconds(m_config.shutdownTimeoutMs);

            for (std::thread& worker : m_workers) {
                if (worker.joinable()) {
                    if (std::chrono::steady_clock::now() < endTime) {
                        worker.join();
                    } else {
                        LOG_WARNING("Thread pool shutdown timed out");
                        break;
                    }
                }
            }
        } else {
            // Wait indefinitely
            for (std::thread& worker : m_workers) {
                if (worker.joinable()) {
                    worker.join();
                }
            }
        }

        // Clear any remaining tasks
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            while (!m_tasks.empty()) {
                auto task = m_tasks.top();
                task->Cancel();
                m_tasks.pop();
                m_stats.IncrementTasksCancelled();
            }
        }

        m_workers.clear();
        m_pendingTasks = 0;
        m_activeThreads = 0;

        LOG_INFO("Thread pool shutdown completed");
    }

    ThreadPool::~ThreadPool() {
        if (IsRunning()) {
            Shutdown();
        }
    }

    void ThreadPool::WorkerFunction() {
        while (true) {
            ThreadTaskPtr task;
            bool hasTask = false;

            {
                std::unique_lock<std::mutex> lock(m_queueMutex);

                // Wait for task or shutdown
                m_condition.wait(
                    lock, [this] { return m_stopping || !m_tasks.empty(); });

                // Check if we should stop
                if (m_stopping && m_tasks.empty()) {
                    return;
                }

                // Get next task
                if (!m_tasks.empty()) {
                    task = m_tasks.top();
                    m_tasks.pop();
                    hasTask = true;
                }
            }

            if (hasTask) {
                // Execute task if not cancelled
                if (!task->IsCancelled()) {
                    m_activeThreads++;
                    m_stats.UpdatePeakActiveThreads(m_activeThreads);

                    try {
                        task->Execute();
                        m_stats.IncrementTasksExecuted();
                    } catch (const std::exception& e) {
                        if (m_config.catchExceptions) {
                            LOG_ERROR("Exception in thread pool task: {}",
                                      e.what());
                            m_stats.IncrementExceptions();
                        } else {
                            throw;
                        }
                    } catch (...) {
                        if (m_config.catchExceptions) {
                            LOG_ERROR("Unknown exception in thread pool task");
                            m_stats.IncrementExceptions();
                        } else {
                            throw;
                        }
                    }

                    m_activeThreads--;
                } else {
                    m_stats.IncrementTasksCancelled();
                }

                m_pendingTasks--;
            }

            // Work stealing (if enabled)
            if (m_config.enableWorkStealing) {
                // Note: Work stealing implementation would go here
                // This is a placeholder for future implementation
            }
        }
    }

    size_t ThreadPool::GetPendingTaskCount() const {
        return m_pendingTasks.load();
    }

    size_t ThreadPool::GetThreadCount() const { return m_workers.size(); }

    void ThreadPool::WaitForCompletion() {
        while (true) {
            {
                std::lock_guard<std::mutex> lock(m_queueMutex);
                if (m_tasks.empty() && m_activeThreads == 0) {
                    break;
                }
            }
            std::this_thread::yield();
        }
    }

}  // namespace Engine
