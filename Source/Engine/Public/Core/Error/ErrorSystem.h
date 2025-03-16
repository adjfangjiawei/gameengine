
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Core/CoreTypes.h"

namespace Engine {

    // Error severity levels
    enum class ErrorSeverity {
        Info,     // Informational message
        Warning,  // Warning that doesn't stop execution
        Error,    // Serious error that might affect execution
        Fatal     // Critical error that should stop execution
    };

    // Error category for better organization
    enum class ErrorCategory {
        None,
        System,
        Memory,
        File,
        Network,
        Render,
        Audio,
        Input,
        Physics,
        Script,
        Custom
    };

    // Struct to hold error context information
    struct ErrorContext {
        std::string file;        // Source file where error occurred
        int32 line;              // Line number where error occurred
        std::string function;    // Function name where error occurred
        std::string message;     // Detailed error message
        ErrorSeverity severity;  // Error severity level
        ErrorCategory category;  // Error category
        uint64 timestamp;        // Error occurrence timestamp

        ErrorContext(const std::string& file,
                     int32 line,
                     const std::string& function,
                     const std::string& message,
                     ErrorSeverity severity = ErrorSeverity::Error,
                     ErrorCategory category = ErrorCategory::None);
    };

    // Error handling callback type
    using ErrorCallback = std::function<void(const ErrorContext&)>;

    class ErrorSystem {
      public:
        static ErrorSystem& Get();

        // Register an error handler
        void RegisterHandler(ErrorCallback handler);

        // Remove an error handler
        void UnregisterHandler(ErrorCallback handler);

        // Report an error
        void ReportError(const ErrorContext& context);

        // Get the last error context
        const ErrorContext* GetLastError() const;

        // Clear error history
        void ClearErrors();

        // Get error history
        const std::vector<ErrorContext>& GetErrorHistory() const;

        // Set maximum number of errors to keep in history
        void SetMaxHistorySize(size_t size);

      private:
        ErrorSystem() = default;
        ~ErrorSystem() = default;

        ErrorSystem(const ErrorSystem&) = delete;
        ErrorSystem& operator=(const ErrorSystem&) = delete;

        std::vector<ErrorCallback> m_handlers;
        std::vector<ErrorContext> m_errorHistory;
        size_t m_maxHistorySize = 100;
    };

// Convenience macros for error reporting
#define REPORT_ERROR(message, severity, category)                \
    Engine::ErrorSystem::Get().ReportError(Engine::ErrorContext( \
        __FILE__, __LINE__, __FUNCTION__, message, severity, category))

#define REPORT_INFO(message) \
    REPORT_ERROR(            \
        message, Engine::ErrorSeverity::Info, Engine::ErrorCategory::None)
#define REPORT_WARNING(message) \
    REPORT_ERROR(               \
        message, Engine::ErrorSeverity::Warning, Engine::ErrorCategory::None)
#define REPORT_ERROR_SIMPLE(message) \
    REPORT_ERROR(                    \
        message, Engine::ErrorSeverity::Error, Engine::ErrorCategory::None)
#define REPORT_FATAL(message) \
    REPORT_ERROR(             \
        message, Engine::ErrorSeverity::Fatal, Engine::ErrorCategory::None)

}  // namespace Engine
