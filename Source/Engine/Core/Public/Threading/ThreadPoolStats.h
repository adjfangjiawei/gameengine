
#pragma once

#include <atomic>
#include <chrono>

#include "CoreTypes.h"

namespace Engine {

    /**
     * @brief Statistics and metrics for thread pool monitoring
     */
    class ThreadPoolStats {
      public:
        /**
         * @brief Reset all statistics to zero
         */
        void Reset() {
            m_totalTasksExecuted = 0;
            m_totalTasksCancelled = 0;
            m_totalExceptions = 0;
            m_peakQueueSize = 0;
            m_peakActiveThreads = 0;
            m_totalWaitTimeUs = 0;
            m_totalExecutionTimeUs = 0;
        }

        // Getters
        uint64_t GetTotalTasksExecuted() const { return m_totalTasksExecuted; }
        uint64_t GetTotalTasksCancelled() const {
            return m_totalTasksCancelled;
        }
        uint64_t GetTotalExceptions() const { return m_totalExceptions; }
        size_t GetPeakQueueSize() const { return m_peakQueueSize; }
        size_t GetPeakActiveThreads() const { return m_peakActiveThreads; }

        /**
         * @brief Get average task wait time in microseconds
         */
        double GetAverageWaitTime() const {
            uint64_t tasks = m_totalTasksExecuted.load();
            if (tasks == 0) return 0.0;
            return static_cast<double>(m_totalWaitTimeUs.load()) / tasks;
        }

        /**
         * @brief Get average task execution time in microseconds
         */
        double GetAverageExecutionTime() const {
            uint64_t tasks = m_totalTasksExecuted.load();
            if (tasks == 0) return 0.0;
            return static_cast<double>(m_totalExecutionTimeUs.load()) / tasks;
        }

        /**
         * @brief Get total wait time in microseconds
         */
        std::chrono::microseconds GetTotalWaitTime() const {
            return std::chrono::microseconds(m_totalWaitTimeUs.load());
        }

        /**
         * @brief Get total execution time in microseconds
         */
        std::chrono::microseconds GetTotalExecutionTime() const {
            return std::chrono::microseconds(m_totalExecutionTimeUs.load());
        }

        // Update methods
        void IncrementTasksExecuted() { ++m_totalTasksExecuted; }
        void IncrementTasksCancelled() { ++m_totalTasksCancelled; }
        void IncrementExceptions() { ++m_totalExceptions; }

        void UpdatePeakQueueSize(size_t currentSize) {
            size_t peak = m_peakQueueSize.load();
            while (currentSize > peak) {
                if (m_peakQueueSize.compare_exchange_weak(peak, currentSize)) {
                    break;
                }
            }
        }

        void UpdatePeakActiveThreads(size_t currentActive) {
            size_t peak = m_peakActiveThreads.load();
            while (currentActive > peak) {
                if (m_peakActiveThreads.compare_exchange_weak(peak,
                                                              currentActive)) {
                    break;
                }
            }
        }

        /**
         * @brief Add wait time to total
         * @param time Duration to add
         */
        void AddWaitTime(std::chrono::microseconds time) {
            uint64_t timeUs = time.count();
            uint64_t current = m_totalWaitTimeUs.load();
            while (!m_totalWaitTimeUs.compare_exchange_weak(current,
                                                            current + timeUs)) {
                // Loop continues until successful update
            }
        }

        /**
         * @brief Add execution time to total
         * @param time Duration to add
         */
        void AddExecutionTime(std::chrono::microseconds time) {
            uint64_t timeUs = time.count();
            uint64_t current = m_totalExecutionTimeUs.load();
            while (!m_totalExecutionTimeUs.compare_exchange_weak(
                current, current + timeUs)) {
                // Loop continues until successful update
            }
        }

      private:
        std::atomic<uint64_t> m_totalTasksExecuted{0};
        std::atomic<uint64_t> m_totalTasksCancelled{0};
        std::atomic<uint64_t> m_totalExceptions{0};
        std::atomic<size_t> m_peakQueueSize{0};
        std::atomic<size_t> m_peakActiveThreads{0};
        std::atomic<uint64_t> m_totalWaitTimeUs{
            0};  // Store microseconds as integer
        std::atomic<uint64_t> m_totalExecutionTimeUs{
            0};  // Store microseconds as integer
    };

}  // namespace Engine
