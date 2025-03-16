
#pragma once

#include <atomic>
#include <functional>
#include <future>
#include <memory>

namespace Engine {

    /**
     * @brief Priority levels for thread pool tasks
     */
    enum class TaskPriority { Low = 0, Normal = 1, High = 2, Critical = 3 };

    /**
     * @brief Represents a cancellable task in the thread pool
     */
    class ThreadTask {
      public:
        using TaskFunction = std::function<void()>;

        /**
         * @brief Create a new task with the given priority
         * @param func The function to execute
         * @param priority The task priority
         */
        ThreadTask(TaskFunction func,
                   TaskPriority priority = TaskPriority::Normal)
            : m_function(std::move(func)),
              m_priority(priority),
              m_cancelled(false) {}

        /**
         * @brief Execute the task if it hasn't been cancelled
         */
        void Execute() {
            if (!IsCancelled() && m_function) {
                m_function();
            }
        }

        /**
         * @brief Cancel the task
         * @return True if the task was cancelled, false if it was already
         * executed or cancelled
         */
        bool Cancel() { return !m_cancelled.exchange(true); }

        /**
         * @brief Check if the task has been cancelled
         * @return True if the task was cancelled
         */
        bool IsCancelled() const { return m_cancelled; }

        /**
         * @brief Get the task's priority
         * @return The task priority
         */
        TaskPriority GetPriority() const { return m_priority; }

        /**
         * @brief Comparison operator for priority queue
         */
        bool operator<(const ThreadTask& other) const {
            return m_priority < other.m_priority;
        }

      private:
        TaskFunction m_function;
        TaskPriority m_priority;
        std::atomic<bool> m_cancelled;
    };

    /**
     * @brief Shared pointer type for thread tasks
     */
    using ThreadTaskPtr = std::shared_ptr<ThreadTask>;

    /**
     * @brief Comparison operator for thread task pointers in priority queue
     */
    struct ThreadTaskPtrCompare {
        bool operator()(const ThreadTaskPtr& a, const ThreadTaskPtr& b) const {
            // Higher priority tasks should come first
            return a->GetPriority() < b->GetPriority();
        }
    };

}  // namespace Engine
