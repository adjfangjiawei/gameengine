
#pragma once

#include <chrono>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "CoreTypes.h"
#include "Log/LogCommon.h"
#include "String/StringUtils.h"

namespace Engine {
    /**
     * @brief Log formatting options
     */
    struct LogFormat {
        bool showTimestamp = true;
        bool showLevel = true;
        bool showCategory = true;
        bool showThreadId = true;
        bool showLocation = true;
        std::string timestampFormat = "%Y-%m-%d %H:%M:%S.%ms";
        std::string messageFormat =
            "[{timestamp}][{level}][{category}][{thread}] {message}";
    };

    /**
     * @brief Logger class for a specific category
     */
    class Logger {
      public:
        Logger(const std::string &category);

        void Log(LogLevel level,
                 const std::string &message,
                 const char *file = "",
                 int line = 0,
                 const char *function = "");

        template <typename... Args>
        void Trace(const std::string &format, Args &&...args);

        template <typename... Args>
        void Debug(const std::string &format, Args &&...args);

        template <typename... Args>
        void Info(const std::string &format, Args &&...args);

        template <typename... Args>
        void Warning(const std::string &format, Args &&...args);

        template <typename... Args>
        void Error(const std::string &format, Args &&...args);

        template <typename... Args>
        void Fatal(const std::string &format, Args &&...args);

      private:
        std::string m_category;
    };

    /**
     * @brief Main log system class
     */
    class LogSystem {
      public:
        static LogSystem &Get();

        void Initialize();
        void Shutdown();

        void AddSink(std::unique_ptr<ILogSink> sink);
        void RemoveSink(ILogSink *sink);

        void SetLogLevel(LogLevel level);
        LogLevel GetLogLevel() const;

        void SetFormat(const LogFormat &format);
        const LogFormat &GetFormat() const;

        Logger &GetLogger(const std::string &category);

        void Log(const LogMessage &message);
        void Flush();

        // Log level specific methods
        template <typename... Args>
        void LogTrace(const std::string &format, Args &&...args) {
            GetLogger("Default").Trace(format, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void LogDebug(const std::string &format, Args &&...args) {
            GetLogger("Default").Debug(format, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void LogInfo(const std::string &format, Args &&...args) {
            GetLogger("Default").Info(format, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void LogWarning(const std::string &format, Args &&...args) {
            GetLogger("Default").Warning(format, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void LogError(const std::string &format, Args &&...args) {
            GetLogger("Default").Error(format, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void LogFatal(const std::string &format, Args &&...args) {
            GetLogger("Default").Fatal(format, std::forward<Args>(args)...);
        }

        std::string FormatMessage(const LogMessage &message) const;

      private:
        LogSystem();
        ~LogSystem();

        LogSystem(const LogSystem &) = delete;
        LogSystem &operator=(const LogSystem &) = delete;

        bool ShouldLog(LogLevel level) const;

      private:
        std::vector<std::unique_ptr<ILogSink>> m_sinks;
        std::unordered_map<std::string, std::unique_ptr<Logger>> m_loggers;
        LogFormat m_format;
        LogLevel m_minLevel;
        mutable std::mutex m_mutex;
        friend class ILogSink;
        friend class FileLogSink;
        friend class ConsoleLogSink;
    };

#define LOG_DEBUG(...) Engine::LogSystem::Get().LogDebug(__VA_ARGS__)
#define LOG_INFO(...) Engine::LogSystem::Get().LogInfo(__VA_ARGS__)
#define LOG_WARNING(...) Engine::LogSystem::Get().LogWarning(__VA_ARGS__)
#define LOG_ERROR(...) Engine::LogSystem::Get().LogError(__VA_ARGS__)
#define LOG_FATAL(...) Engine::LogSystem::Get().LogFatal(__VA_ARGS__)
#define LOG_TRACE(...) Engine::LogSystem::Get().LogTrace(__VA_ARGS__)
#define LOG_TRACE_IF(condition, ...)                    \
    if (condition) {                                    \
        Engine::LogSystem::Get().LogTrace(__VA_ARGS__); \
    }

    // Template implementations
    template <typename... Args>
    void Logger::Trace(const std::string &format, Args &&...args) {
        Log(LogLevel::Trace,
            StringUtils::Format(format.c_str(), std::forward<Args>(args)...),
            __FILE__,
            __LINE__,
            __FUNCTION__);
    }

    template <typename... Args>
    void Logger::Debug(const std::string &format, Args &&...args) {
        Log(LogLevel::Debug,
            StringUtils::Format(format.c_str(), std::forward<Args>(args)...),
            __FILE__,
            __LINE__,
            __FUNCTION__);
    }

    template <typename... Args>
    void Logger::Info(const std::string &format, Args &&...args) {
        Log(LogLevel::Info,
            StringUtils::Format(format.c_str(), std::forward<Args>(args)...),
            __FILE__,
            __LINE__,
            __FUNCTION__);
    }

    template <typename... Args>
    void Logger::Warning(const std::string &format, Args &&...args) {
        Log(LogLevel::Warning,
            StringUtils::Format(format.c_str(), std::forward<Args>(args)...),
            __FILE__,
            __LINE__,
            __FUNCTION__);
    }

    template <typename... Args>
    void Logger::Error(const std::string &format, Args &&...args) {
        Log(LogLevel::Error,
            StringUtils::Format(format.c_str(), std::forward<Args>(args)...),
            __FILE__,
            __LINE__,
            __FUNCTION__);
    }

    template <typename... Args>
    void Logger::Fatal(const std::string &format, Args &&...args) {
        Log(LogLevel::Fatal,
            StringUtils::Format(format.c_str(), std::forward<Args>(args)...),
            __FILE__,
            __LINE__,
            __FUNCTION__);
    }

}  // namespace Engine
