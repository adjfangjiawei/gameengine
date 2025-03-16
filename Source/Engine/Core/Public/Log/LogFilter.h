
#pragma once

#include <memory>
#include <regex>
#include <string>
#include <vector>

#include "Log/LogSystem.h"

namespace Engine {

    /**
     * @brief Interface for log message filters
     */
    class ILogFilter {
      public:
        virtual ~ILogFilter() = default;
        virtual bool ShouldLog(const LogMessage& message) const = 0;
    };

    /**
     * @brief Filter based on log level
     */
    class LogLevelFilter : public ILogFilter {
      public:
        explicit LogLevelFilter(LogLevel minLevel) : m_minLevel(minLevel) {}
        bool ShouldLog(const LogMessage& message) const override {
            return message.level >= m_minLevel;
        }

      private:
        LogLevel m_minLevel;
    };

    /**
     * @brief Filter based on category
     */
    class CategoryFilter : public ILogFilter {
      public:
        void AddCategory(const std::string& category, bool allow = true) {
            m_categories[category] = allow;
        }

        void RemoveCategory(const std::string& category) {
            m_categories.erase(category);
        }

        bool ShouldLog(const LogMessage& message) const override {
            auto it = m_categories.find(message.category);
            return it == m_categories.end() ? true : it->second;
        }

      private:
        std::unordered_map<std::string, bool> m_categories;
    };

    /**
     * @brief Filter based on regular expression pattern
     */
    class PatternFilter : public ILogFilter {
      public:
        explicit PatternFilter(const std::string& pattern, bool allow = true)
            : m_pattern(pattern), m_allow(allow) {}

        bool ShouldLog(const LogMessage& message) const override {
            bool matches = std::regex_search(message.message, m_pattern);
            return matches ? m_allow : !m_allow;
        }

      private:
        std::regex m_pattern;
        bool m_allow;
    };

    /**
     * @brief Composite filter that combines multiple filters
     */
    class FilterChain : public ILogFilter {
      public:
        enum class Operation {
            And,  // All filters must accept
            Or    // At least one filter must accept
        };

        explicit FilterChain(Operation op = Operation::And) : m_operation(op) {}

        void AddFilter(std::unique_ptr<ILogFilter> filter) {
            m_filters.push_back(std::move(filter));
        }

        bool ShouldLog(const LogMessage& message) const override {
            if (m_filters.empty()) {
                return true;
            }

            if (m_operation == Operation::And) {
                for (const auto& filter : m_filters) {
                    if (!filter->ShouldLog(message)) {
                        return false;
                    }
                }
                return true;
            } else {  // Operation::Or
                for (const auto& filter : m_filters) {
                    if (filter->ShouldLog(message)) {
                        return true;
                    }
                }
                return false;
            }
        }

      private:
        Operation m_operation;
        std::vector<std::unique_ptr<ILogFilter>> m_filters;
    };

}  // namespace Engine
