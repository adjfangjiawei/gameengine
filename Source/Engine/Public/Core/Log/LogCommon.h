#pragma once

#include <chrono>
#include <string>
#include <thread>

namespace Engine {

    /**
     * @brief Log severity levels
     */
    enum class LogLevel {
        Trace,    // Detailed information for debugging
        Debug,    // Debugging information
        Info,     // General information
        Warning,  // Warnings that don't prevent execution
        Error,    // Errors that may affect execution
        Fatal     // Critical errors that prevent execution
    };

    /**
     * @brief Log message structure
     */
    struct LogMessage {
        LogLevel level;
        std::string message;
        std::string category;
        std::string file;
        int line;
        const char* function;
        std::chrono::system_clock::time_point timestamp;
        std::thread::id threadId;
    };

    /**
     * @brief Interface for log sinks
     */
    class ILogSink {
      public:
        virtual ~ILogSink() = default;
        virtual void Write(const LogMessage& message) = 0;
        virtual void Flush() = 0;
    };

}  // namespace Engine
