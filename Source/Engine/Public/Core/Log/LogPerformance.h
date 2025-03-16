
#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <vector>

#include "Core/Log/LogSystem.h"

namespace Engine {

    /**
     * @brief Object pool for LogMessage objects to reduce memory allocations
     */
    class LogMessagePool {
      public:
        static constexpr size_t DEFAULT_POOL_SIZE = 1024;

        explicit LogMessagePool(size_t poolSize = DEFAULT_POOL_SIZE)
            : m_messages(poolSize) {
            for (size_t i = 0; i < poolSize; ++i) {
                m_freeMessages.push(&m_messages[i]);
            }
        }

        LogMessage* Acquire() {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_freeMessages.empty()) {
                // Pool is exhausted, create a new message
                m_messages.push_back(LogMessage{});
                return &m_messages.back();
            }

            LogMessage* message = m_freeMessages.front();
            m_freeMessages.pop();
            return message;
        }

        void Release(LogMessage* message) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_freeMessages.push(message);
        }

      private:
        std::vector<LogMessage> m_messages;
        std::queue<LogMessage*> m_freeMessages;
        std::mutex m_mutex;
        std::condition_variable m_condition;
        std::condition_variable m_flushComplete;
        std::thread m_processingThread;
        [[maybe_unused]] bool m_stopping{false};
    };

    /**
     * @brief Batch writer for log messages
     */
    class BatchLogSink : public ILogSink {
      public:
        static constexpr size_t DEFAULT_BATCH_SIZE = 100;
        static constexpr auto DEFAULT_FLUSH_INTERVAL = std::chrono::seconds(1);

        BatchLogSink(
            std::unique_ptr<ILogSink> sink,
            size_t batchSize = DEFAULT_BATCH_SIZE,
            std::chrono::milliseconds flushInterval = DEFAULT_FLUSH_INTERVAL);

        ~BatchLogSink();

        void Write(const LogMessage& message) override;
        void Flush() override;

      private:
        void ProcessBatch();
        bool ShouldFlush() const;

      private:
        std::unique_ptr<ILogSink> m_sink;
        std::vector<LogMessage> m_batch;
        size_t m_batchSize;
        std::chrono::milliseconds m_flushInterval;
        std::chrono::steady_clock::time_point m_lastFlush;
        std::mutex m_mutex;
        std::condition_variable m_condition;
        std::condition_variable m_flushComplete;
        std::thread m_processingThread;
        bool m_stopping{false};
    };

    /**
     * @brief Performance metrics for the logging system
     */
    class LogPerformanceMetrics {
      public:
        void RecordMessageProcessed() {
            ++m_messageCount;
            UpdateThroughput();
        }

        void RecordMessageDropped() { ++m_droppedCount; }

        void RecordProcessingTime(std::chrono::nanoseconds duration) {
            m_totalProcessingTime += duration.count();
            ++m_processingCount;

            auto avgTime = m_totalProcessingTime / m_processingCount;
            m_averageProcessingTime.store(avgTime, std::memory_order_relaxed);
        }

        // Getters for metrics
        uint64_t GetMessageCount() const { return m_messageCount.load(); }
        uint64_t GetDroppedCount() const { return m_droppedCount.load(); }
        double GetAverageProcessingTime() const {
            return m_averageProcessingTime.load();
        }
        double GetThroughput() const { return m_throughput.load(); }

      private:
        void UpdateThroughput() {
            auto now = std::chrono::steady_clock::now();
            auto duration = now - m_lastThroughputUpdate;

            if (duration >= std::chrono::seconds(1)) {
                uint64_t currentCount = m_messageCount.load();
                uint64_t difference = currentCount - m_lastMessageCount;

                double throughput =
                    static_cast<double>(difference) /
                    std::chrono::duration<double>(duration).count();

                m_throughput.store(throughput, std::memory_order_relaxed);
                m_lastMessageCount = currentCount;
                m_lastThroughputUpdate = now;
            }
        }

      private:
        std::atomic<uint64_t> m_messageCount{0};
        std::atomic<uint64_t> m_droppedCount{0};
        std::atomic<uint64_t> m_processingCount{0};
        std::atomic<uint64_t> m_totalProcessingTime{0};
        std::atomic<double> m_averageProcessingTime{0.0};
        std::atomic<double> m_throughput{0.0};

        uint64_t m_lastMessageCount{0};
        std::chrono::steady_clock::time_point m_lastThroughputUpdate{
            std::chrono::steady_clock::now()};
    };

}  // namespace Engine
