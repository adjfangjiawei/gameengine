
#include "Log/LogSystem.h"

#include <filesystem>
#include <iomanip>
#include <iostream>

#include "Log/LogCommon.h"
#include "Log/LogSinks.h"
#include "String/StringUtils.h"

namespace Engine {

    // Logger implementation
    Logger::Logger(const std::string &category) : m_category(category) {}

    void Logger::Log(LogLevel level,
                     const std::string &message,
                     const char *file,
                     int line,
                     const char *function) {
        LogMessage logMessage{level,
                              message,
                              m_category,
                              file,
                              line,
                              function,
                              std::chrono::system_clock::now(),
                              std::this_thread::get_id()};

        LogSystem::Get().Log(logMessage);
    }

    // LogSystem implementation
    LogSystem::LogSystem() : m_minLevel(LogLevel::Info) {
        m_format.showTimestamp = true;
        m_format.showLevel = true;
        m_format.showCategory = true;
        m_format.showThreadId = true;
        m_format.showLocation = true;
    }

    LogSystem::~LogSystem() { Shutdown(); }

    LogSystem &LogSystem::Get() {
        static LogSystem instance;
        return instance;
    }

    void LogSystem::Initialize() {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        // Add default console sink with colors
        AddSink(std::make_unique<ConsoleLogSink>(true));

        // Create logs directory if it doesn't exist
        std::filesystem::create_directories("Saved/Logs");

        // Add default file sink
        std::unique_ptr<ILogSink> fileSink(
            (ILogSink *)(new FileLogSink("Saved/Logs/Game.log", true)));
        AddSink(std::move(fileSink));
    }

    void LogSystem::Shutdown() {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        Flush();
        m_sinks.clear();
        m_loggers.clear();
    }

    void LogSystem::AddSink(std::unique_ptr<ILogSink> sink) {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        m_sinks.push_back(std::move(sink));
    }

    void LogSystem::RemoveSink(ILogSink *sink) {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        auto it =
            std::find_if(m_sinks.begin(), m_sinks.end(), [sink](const auto &s) {
                return s.get() == sink;
            });
        if (it != m_sinks.end()) {
            m_sinks.erase(it);
        }
    }

    void LogSystem::SetLogLevel(LogLevel level) {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        m_minLevel = level;
    }

    LogLevel LogSystem::GetLogLevel() const {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        return m_minLevel;
    }

    void LogSystem::SetFormat(const LogFormat &format) {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        m_format = format;
    }

    const LogFormat &LogSystem::GetFormat() const {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        return m_format;
    }

    Logger &LogSystem::GetLogger(const std::string &category) {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        Initialize();
        auto it = m_loggers.find(category);
        if (it == m_loggers.end()) {
            auto [newIt, inserted] =
                m_loggers.emplace(category, std::make_unique<Logger>(category));
            return *newIt->second;
        }
        return *it->second;
    }

    void LogSystem::Log(const LogMessage &message) {
        if (!ShouldLog(message.level)) {
            return;
        }

        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        for (const auto &sink : m_sinks) {
            sink->Write(message);
        }

        if (message.level >= LogLevel::Error) {
            Flush();
        }
    }

    void LogSystem::Flush() {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        for (const auto &sink : m_sinks) {
            sink->Flush();
        }
    }

    std::string LogSystem::FormatMessage(const LogMessage &message) const {
        std::string result = m_format.messageFormat;

        if (m_format.showTimestamp) {
            auto time = std::chrono::system_clock::to_time_t(message.timestamp);
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          message.timestamp.time_since_epoch())
                          .count() %
                      1000;

            std::stringstream ss;
            ss << std::put_time(std::localtime(&time),
                                m_format.timestampFormat.c_str());
            char msStr[8];
            snprintf(msStr, sizeof(msStr), "%03d", static_cast<int>(ms));
            std::string timestamp =
                StringUtils::Replace(ss.str(), "%ms", msStr);

            result = StringUtils::Replace(result, "{timestamp}", timestamp);
        }

        if (m_format.showLevel) {
            std::string level;
            switch (message.level) {
                case LogLevel::Trace:
                    level = "TRACE";
                    break;
                case LogLevel::Debug:
                    level = "DEBUG";
                    break;
                case LogLevel::Info:
                    level = "INFO";
                    break;
                case LogLevel::Warning:
                    level = "WARNING";
                    break;
                case LogLevel::Error:
                    level = "ERROR";
                    break;
                case LogLevel::Fatal:
                    level = "FATAL";
                    break;
            }
            result = StringUtils::Replace(result, "{level}", level);
        }

        if (m_format.showCategory) {
            result =
                StringUtils::Replace(result, "{category}", message.category);
        }

        if (m_format.showThreadId) {
            std::stringstream ss;
            ss << message.threadId;
            result = StringUtils::Replace(result, "{thread}", ss.str());
        }

        if (m_format.showLocation && !message.file.empty()) {
            std::stringstream locationStream;
            locationStream << message.file << ":" << message.line << " ("
                           << message.function << ")";
            std::string location = locationStream.str();
            result = StringUtils::Replace(result, "{location}", location);
        }

        result = StringUtils::Replace(result, "{message}", message.message);
        return result;
    }

    bool LogSystem::ShouldLog(LogLevel level) const {
        return level >= m_minLevel;
    }

}  // namespace Engine
