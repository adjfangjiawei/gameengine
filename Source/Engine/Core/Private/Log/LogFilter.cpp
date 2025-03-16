
#include "Log/LogFilter.h"

#include <algorithm>
#include <cctype>

#include "String/StringUtils.h"

namespace Engine {

    namespace {
        // Helper function to check if a string matches a wildcard pattern
        bool MatchWildcard(const std::string& text,
                           const std::string& pattern) {
            std::vector<std::vector<bool>> dp(
                text.length() + 1,
                std::vector<bool>(pattern.length() + 1, false));
            dp[0][0] = true;

            for (size_t j = 1; j <= pattern.length(); ++j) {
                if (pattern[j - 1] == '*') {
                    dp[0][j] = dp[0][j - 1];
                }
            }

            for (size_t i = 1; i <= text.length(); ++i) {
                for (size_t j = 1; j <= pattern.length(); ++j) {
                    if (pattern[j - 1] == '*') {
                        dp[i][j] = dp[i][j - 1] || dp[i - 1][j];
                    } else if (pattern[j - 1] == '?' ||
                               text[i - 1] == pattern[j - 1]) {
                        dp[i][j] = dp[i - 1][j - 1];
                    }
                }
            }

            return dp[text.length()][pattern.length()];
        }
    }  // namespace

    /**
     * @brief Enhanced pattern filter that supports wildcards
     */
    class WildcardPatternFilter : public ILogFilter {
      public:
        WildcardPatternFilter(const std::string& pattern, bool allow = true)
            : m_pattern(pattern), m_allow(allow) {}

        bool ShouldLog(const LogMessage& message) const override {
            bool matches = MatchWildcard(message.message, m_pattern);
            return matches ? m_allow : !m_allow;
        }

      private:
        std::string m_pattern;
        bool m_allow;
    };

    /**
     * @brief Filter that combines multiple patterns with AND/OR logic
     */
    class MultiPatternFilter : public ILogFilter {
      public:
        enum class Logic { And, Or };

        MultiPatternFilter(Logic logic = Logic::And) : m_logic(logic) {}

        void AddPattern(const std::string& pattern, bool allow = true) {
            m_patterns.emplace_back(pattern, allow);
        }

        bool ShouldLog(const LogMessage& message) const override {
            if (m_patterns.empty()) {
                return true;
            }

            if (m_logic == Logic::And) {
                return std::all_of(m_patterns.begin(),
                                   m_patterns.end(),
                                   [&message](const auto& pattern) {
                                       return MatchWildcard(message.message,
                                                            pattern.first) ==
                                              pattern.second;
                                   });
            } else {
                return std::any_of(m_patterns.begin(),
                                   m_patterns.end(),
                                   [&message](const auto& pattern) {
                                       return MatchWildcard(message.message,
                                                            pattern.first) ==
                                              pattern.second;
                                   });
            }
        }

      private:
        Logic m_logic;
        std::vector<std::pair<std::string, bool>> m_patterns;
    };

    /**
     * @brief Filter based on time range
     */
    class TimeRangeFilter : public ILogFilter {
      public:
        TimeRangeFilter(std::chrono::system_clock::time_point start,
                        std::chrono::system_clock::time_point end)
            : m_start(start), m_end(end) {}

        bool ShouldLog(const LogMessage& message) const override {
            return message.timestamp >= m_start && message.timestamp <= m_end;
        }

      private:
        std::chrono::system_clock::time_point m_start;
        std::chrono::system_clock::time_point m_end;
    };

    /**
     * @brief Filter based on message rate
     */
    class RateFilter : public ILogFilter {
      public:
        RateFilter(size_t maxMessages, std::chrono::seconds window)
            : m_maxMessages(maxMessages), m_window(window) {}

        bool ShouldLog(
            [[maybe_unused]] const LogMessage& message) const override {
            auto now = std::chrono::system_clock::now();

            // Remove old messages
            while (!m_messageTimestamps.empty() &&
                   m_messageTimestamps.front() < now - m_window) {
                m_messageTimestamps.pop_front();
            }

            // Check if we're under the rate limit
            if (m_messageTimestamps.size() < m_maxMessages) {
                m_messageTimestamps.push_back(now);
                return true;
            }

            return false;
        }

      private:
        size_t m_maxMessages;
        std::chrono::seconds m_window;
        mutable std::deque<std::chrono::system_clock::time_point>
            m_messageTimestamps;
    };

    /**
     * @brief Filter factory to create filters from configuration
     */
    class LogFilterFactory {
      public:
        static std::unique_ptr<ILogFilter> CreateFilter(
            const std::string& type,
            const std::unordered_map<std::string, std::string>& params) {
            if (type == "level") {
                auto levelIt = params.find("min_level");
                if (levelIt != params.end()) {
                    return std::make_unique<LogLevelFilter>(
                        ParseLogLevel(levelIt->second));
                }
            } else if (type == "category") {
                auto filter = std::make_unique<CategoryFilter>();
                for (const auto& [category, allowStr] : params) {
                    filter->AddCategory(category,
                                        StringUtils::ToBool(allowStr));
                }
                return filter;
            } else if (type == "pattern") {
                auto patternIt = params.find("pattern");
                auto allowIt = params.find("allow");
                if (patternIt != params.end()) {
                    bool allow = allowIt != params.end()
                                     ? StringUtils::ToBool(allowIt->second)
                                     : true;
                    return std::make_unique<PatternFilter>(patternIt->second,
                                                           allow);
                }
            } else if (type == "wildcard") {
                auto patternIt = params.find("pattern");
                auto allowIt = params.find("allow");
                if (patternIt != params.end()) {
                    bool allow = allowIt != params.end()
                                     ? StringUtils::ToBool(allowIt->second)
                                     : true;
                    return std::make_unique<WildcardPatternFilter>(
                        patternIt->second, allow);
                }
            } else if (type == "rate") {
                auto maxMessagesIt = params.find("max_messages");
                auto windowIt = params.find("window_seconds");
                if (maxMessagesIt != params.end() && windowIt != params.end()) {
                    return std::make_unique<RateFilter>(
                        std::stoull(maxMessagesIt->second),
                        std::chrono::seconds(std::stoull(windowIt->second)));
                }
            }

            // Return nullptr if filter creation fails
            return nullptr;
        }

      private:
        static LogLevel ParseLogLevel(const std::string& level) {
            if (StringUtils::EqualsIgnoreCase(level, "trace"))
                return LogLevel::Trace;
            if (StringUtils::EqualsIgnoreCase(level, "debug"))
                return LogLevel::Debug;
            if (StringUtils::EqualsIgnoreCase(level, "info"))
                return LogLevel::Info;
            if (StringUtils::EqualsIgnoreCase(level, "warning"))
                return LogLevel::Warning;
            if (StringUtils::EqualsIgnoreCase(level, "error"))
                return LogLevel::Error;
            if (StringUtils::EqualsIgnoreCase(level, "fatal"))
                return LogLevel::Fatal;
            return LogLevel::Info;
        }
    };

}  // namespace Engine
