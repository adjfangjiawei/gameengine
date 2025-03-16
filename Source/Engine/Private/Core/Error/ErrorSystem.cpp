
#include "Core/Error/ErrorSystem.h"

#include <algorithm>

#include "Core/Time.h"

namespace Engine {

    ErrorContext::ErrorContext(const std::string& file,
                               int32 line,
                               const std::string& function,
                               const std::string& message,
                               ErrorSeverity severity,
                               ErrorCategory category)
        : file(file),
          line(line),
          function(function),
          message(message),
          severity(severity),
          category(category),
          timestamp(std::chrono::duration_cast<std::chrono::milliseconds>(
                        Time::Now().time_since_epoch())
                        .count()) {}

    ErrorSystem& ErrorSystem::Get() {
        static ErrorSystem instance;
        return instance;
    }

    void ErrorSystem::RegisterHandler(ErrorCallback handler) {
        if (handler) {
            // Check if handler already exists
            auto it = std::find_if(
                m_handlers.begin(),
                m_handlers.end(),
                [&handler](const ErrorCallback& existing) {
                    return existing.target<void(const ErrorContext&)>() ==
                           handler.target<void(const ErrorContext&)>();
                });

            if (it == m_handlers.end()) {
                m_handlers.push_back(handler);
            }
        }
    }

    void ErrorSystem::UnregisterHandler(ErrorCallback handler) {
        if (handler) {
            m_handlers.erase(
                std::remove_if(
                    m_handlers.begin(),
                    m_handlers.end(),
                    [&handler](const ErrorCallback& existing) {
                        return existing.target<void(const ErrorContext&)>() ==
                               handler.target<void(const ErrorContext&)>();
                    }),
                m_handlers.end());
        }
    }

    void ErrorSystem::ReportError(const ErrorContext& context) {
        // Add to history
        m_errorHistory.push_back(context);

        // Maintain history size
        if (m_errorHistory.size() > m_maxHistorySize) {
            m_errorHistory.erase(m_errorHistory.begin());
        }

        // Notify all handlers
        for (const auto& handler : m_handlers) {
            if (handler) {
                handler(context);
            }
        }

        // For fatal errors, we might want to take additional actions
        if (context.severity == ErrorSeverity::Fatal) {
            // TODO: Implement fatal error handling strategy
            // This could include:
            // - Logging to a crash dump
            // - Triggering the crash reporter
            // - Initiating a graceful shutdown
        }
    }

    const ErrorContext* ErrorSystem::GetLastError() const {
        if (m_errorHistory.empty()) {
            return nullptr;
        }
        return &m_errorHistory.back();
    }

    void ErrorSystem::ClearErrors() { m_errorHistory.clear(); }

    const std::vector<ErrorContext>& ErrorSystem::GetErrorHistory() const {
        return m_errorHistory;
    }

    void ErrorSystem::SetMaxHistorySize(size_t size) {
        m_maxHistorySize = size;
        if (m_errorHistory.size() > m_maxHistorySize) {
            m_errorHistory.erase(
                m_errorHistory.begin(),
                m_errorHistory.begin() +
                    (m_errorHistory.size() - m_maxHistorySize));
        }
    }

}  // namespace Engine
