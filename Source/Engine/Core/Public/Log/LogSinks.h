
#pragma once

#include <condition_variable>
#include <fstream>
#include <functional>
#include <queue>

#include "FileSystem/FileSystem.h"
#include "Log/LogCommon.h"

namespace Engine {

    /**
     * @brief Console output sink
     */
    class ConsoleLogSink : public ILogSink {
      public:
        ConsoleLogSink(bool useColors = true);
        ~ConsoleLogSink() override {};
        void Write(const LogMessage &message) override;
        void Flush() override;

      private:
        void SetConsoleColor(LogLevel level);
        void ResetConsoleColor();

        bool m_useColors;
        std::mutex m_mutex;
    };

    /**
     * @brief File output sink
     */
    class FileLogSink : public ILogSink {
      public:
        FileLogSink(const std::string &filename,
                    bool append = true,
                    size_t maxFileSize = 10 * 1024 * 1024,  // 10MB
                    size_t maxFiles = 5);
        ~FileLogSink();

        void Write(const LogMessage &message) override;
        void Flush() override;

      private:
        void RollFiles();
        std::string GetBackupFileName(size_t index) const;
        void OpenLogFile(bool append = true);

        std::string m_filename;
        std::ofstream m_file;                 // 保留std::ofstream用于格式化输出
        std::unique_ptr<IFile> m_fileHandle;  // 用于文件操作
        size_t m_maxFileSize;
        size_t m_maxFiles;
        size_t m_currentSize;
        std::mutex m_mutex;
    };

    /**
     * @brief Debug output sink (Windows OutputDebugString or Unix syslog)
     */
    class DebugLogSink : public ILogSink {
      public:
        void Write(const LogMessage &message) override;
        void Flush() override;

      private:
        std::mutex m_mutex;
    };

    /**
     * @brief Rotating file sink that creates new file based on time
     */
    class RotatingFileLogSink : public ILogSink {
      public:
        enum class RotationInterval { Daily, Weekly, Monthly };

        RotatingFileLogSink(
            const std::string &filenamePattern,
            RotationInterval interval = RotationInterval::Daily);
        ~RotatingFileLogSink();

        void Write(const LogMessage &message) override;
        void Flush() override;

      private:
        void CheckRotation();
        std::string GetCurrentFileName() const;
        bool ShouldRotate() const;

        std::string m_filenamePattern;
        RotationInterval m_interval;
        std::unique_ptr<IFile> m_currentFile;
        std::chrono::system_clock::time_point m_nextRotation;
        std::mutex m_mutex;
    };

    /**
     * @brief Memory buffer sink for testing or temporary storage
     */
    class MemoryLogSink : public ILogSink {
      public:
        MemoryLogSink(size_t maxMessages = 1000);

        void Write(const LogMessage &message) override;
        void Flush() override;

        std::vector<LogMessage> GetMessages() const;
        void Clear();

      private:
        size_t m_maxMessages;
        std::queue<LogMessage> m_messages;
        mutable std::mutex m_mutex;
    };

    /**
     * @brief Async log sink that processes messages in a background thread
     */
    class AsyncLogSink : public ILogSink {
      public:
        AsyncLogSink(std::unique_ptr<ILogSink> sink);
        ~AsyncLogSink();

        void Write(const LogMessage &message) override;
        void Flush() override;

      private:
        void ProcessMessages();

        std::unique_ptr<ILogSink> m_sink;
        std::queue<LogMessage> m_messageQueue;
        std::thread m_processingThread;
        std::mutex m_mutex;
        std::condition_variable m_condition;
        bool m_stopping;
    };

    /**
     * @brief Composite sink that writes to multiple sinks
     */
    class CompositLogSink : public ILogSink {
      public:
        void AddSink(std::unique_ptr<ILogSink> sink);
        void RemoveSink(ILogSink *sink);

        void Write(const LogMessage &message) override;
        void Flush() override;

      private:
        std::vector<std::unique_ptr<ILogSink>> m_sinks;
        std::mutex m_mutex;
    };

    /**
     * @brief Filtered sink that only processes messages matching certain
     * criteria
     */
    class FilteredLogSink : public ILogSink {
      public:
        using FilterFunction = std::function<bool(const LogMessage &)>;

        FilteredLogSink(std::unique_ptr<ILogSink> sink, FilterFunction filter);

        void Write(const LogMessage &message) override;
        void Flush() override;

      private:
        std::unique_ptr<ILogSink> m_sink;
        FilterFunction m_filter;
    };

    class RotatingFileSink : public ILogSink {
      public:
        struct Config {
            std::string baseFilename;
            size_t maxFileSize = 10 * 1024 * 1024;  // 10MB default
            size_t maxFiles = 5;
            bool appendTimestamp = true;
        };

        explicit RotatingFileSink(const Config &config);
        ~RotatingFileSink() override;

        void Write(const LogMessage &message) override;

      private:
        void RotateFiles();
        std::string FormatFileName() const;

        Config config;
        std::unique_ptr<IFile> m_currentFile;
        std::ofstream m_fileStream;  // 保留用于格式化输出
        size_t currentFileSize;
        std::mutex mutex;
    };

}  // namespace Engine
