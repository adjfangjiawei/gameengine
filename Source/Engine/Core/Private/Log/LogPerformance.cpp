
#include "Core/Log/LogPerformance.h"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <condition_variable>
namespace Engine {

    namespace {
        // Constants for performance tuning
        constexpr size_t MAX_BATCH_SIZE = 10000;
        constexpr auto MAX_BATCH_DELAY = std::chrono::seconds(5);
        constexpr size_t INITIAL_BUFFER_SIZE = 4096;
    }  // namespace

    BatchLogSink::BatchLogSink(std::unique_ptr<ILogSink> sink,
                               size_t batchSize,
                               std::chrono::milliseconds flushInterval)
        : m_sink(std::move(sink)),
          m_batchSize(std::min<size_t>(batchSize, MAX_BATCH_SIZE)),
          m_flushInterval(std::min<std::chrono::milliseconds>(flushInterval,
                                                              MAX_BATCH_DELAY)),
          m_lastFlush(std::chrono::steady_clock::now()) {
        m_batch.reserve(m_batchSize);
        m_processingThread = std::thread(&BatchLogSink::ProcessBatch, this);
    }

    BatchLogSink::~BatchLogSink() {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_stopping = true;
        }
        m_condition.notify_one();

        if (m_processingThread.joinable()) {
            m_processingThread.join();
        }
    }

    void BatchLogSink::Write(const LogMessage& message) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_batch.push_back(message);

        if (ShouldFlush()) {
            m_condition.notify_one();
        }
    }

    void BatchLogSink::Flush() {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (!m_batch.empty()) {
            m_condition.notify_one();
            m_flushComplete.wait(lock);
        }
    }

    void BatchLogSink::ProcessBatch() {
        std::vector<LogMessage> processingBatch;
        processingBatch.reserve(m_batchSize);

        while (true) {
            {
                std::unique_lock<std::mutex> lock(m_mutex);

                m_condition.wait(
                    lock, [this] { return ShouldFlush() || m_stopping; });

                if (m_stopping && m_batch.empty()) {
                    break;
                }

                processingBatch.swap(m_batch);
                m_lastFlush = std::chrono::steady_clock::now();
            }

            // Process the batch
            for (const auto& message : processingBatch) {
                m_sink->Write(message);
            }

            processingBatch.clear();

            // Notify that flush is complete
            m_flushComplete.notify_all();
        }
    }

    bool BatchLogSink::ShouldFlush() const {
        auto now = std::chrono::steady_clock::now();
        return m_batch.size() >= m_batchSize ||
               (now - m_lastFlush) >= m_flushInterval;
    }

    /**
     * @brief High-performance string formatter for log messages
     */
    class FastLogFormatter {
      public:
        FastLogFormatter() { m_buffer.reserve(INITIAL_BUFFER_SIZE); }

        void Clear() { m_buffer.clear(); }

        FastLogFormatter& Append(const char* str) {
            if (str) {
                m_buffer.append(str);
            }
            return *this;
        }

        FastLogFormatter& Append(const std::string& str) {
            m_buffer.append(str);
            return *this;
        }

        FastLogFormatter& Append(char ch) {
            m_buffer.push_back(ch);
            return *this;
        }

        template <typename T>
        FastLogFormatter& AppendNumber(T value) {
            auto start = m_buffer.size();
            m_buffer.resize(start + 32);  // Reserve space for number
            auto [ptr, ec] = std::to_chars(m_buffer.data() + start,
                                           m_buffer.data() + m_buffer.size(),
                                           value);
            m_buffer.resize(ptr - m_buffer.data());
            return *this;
        }

        const std::string& GetString() const { return m_buffer; }

      private:
        std::string m_buffer;
    };

    /**
     * @brief Thread-local message formatter cache
     */
    class FormatterCache {
      public:
        static FastLogFormatter& Get() {
            static thread_local FastLogFormatter formatter;
            formatter.Clear();
            return formatter;
        }
    };

}  // namespace Engine
