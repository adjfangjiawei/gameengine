
#pragma once

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "Core/Log/LogSystem.h"

namespace Engine {

    /**
     * @brief Statistics for log messages
     */
    struct LogStatistics {
        struct CategoryStats {
            uint64_t messageCount{0};
            std::map<LogLevel, uint64_t> levelCounts;
            std::chrono::system_clock::time_point lastMessage;
            double messagesPerSecond{0.0};
        };

        std::unordered_map<std::string, CategoryStats> categoryStats;
        uint64_t totalMessages{0};
        std::chrono::system_clock::time_point startTime;
        std::chrono::system_clock::time_point lastUpdate;
    };

    /**
     * @brief Alert condition for log monitoring
     */
    struct LogAlertCondition {
        enum class Type {
            MessageRate,      // Messages per second
            ErrorCount,       // Number of errors in time window
            PatternMatch,     // Specific pattern in messages
            CategoryActivity  // Activity in specific category
        };

        Type type;
        std::string category;
        LogLevel level{LogLevel::Info};
        double threshold{0.0};
        std::chrono::seconds timeWindow{60};
        std::string pattern;
        bool enabled{true};
    };

    /**
     * @brief Callback for log alerts
     */
    using LogAlertCallback = std::function<void(
        const std::string& alertMessage, const LogMessage& triggeringMessage)>;

    /**
     * @brief Log analytics and monitoring system
     */
    class LogAnalytics {
      public:
        // Singleton access
        static LogAnalytics& Get();

        // Statistics management
        void UpdateStatistics(const LogMessage& message);
        LogStatistics GetStatistics();
        void ResetStatistics();

        // Alert management
        void AddAlertCondition(const std::string& name,
                               const LogAlertCondition& condition,
                               LogAlertCallback callback);
        void RemoveAlertCondition(const std::string& name);
        void EnableAlertCondition(const std::string& name, bool enable = true);

        // Analysis features
        struct TimeRange {
            std::chrono::system_clock::time_point start;
            std::chrono::system_clock::time_point end;
        };

        // Query and analyze logs
        std::vector<LogMessage> QueryLogs(const TimeRange& range,
                                          const std::string& category = "",
                                          LogLevel minLevel = LogLevel::Trace,
                                          const std::string& pattern = "");

        // Generate reports
        struct LogReport {
            std::string category;
            uint64_t messageCount;
            std::map<LogLevel, uint64_t> levelDistribution;
            double averageRate;
            std::vector<std::string> topPatterns;
            std::vector<TimeRange> activitySpikes;
        };

        LogReport GenerateReport(const TimeRange& range,
                                 const std::string& category = "");

        // Real-time monitoring
        struct MonitoringConfig {
            bool enableRateMonitoring{true};
            bool enablePatternDetection{true};
            bool enableErrorTracking{true};
            double samplingRate{1.0};  // 1.0 = 100%
            std::chrono::seconds aggregationWindow{60};
        };

        void ConfigureMonitoring(const MonitoringConfig& config);
        void StartMonitoring();
        void StopMonitoring();

      private:
        LogAnalytics() = default;
        ~LogAnalytics() = default;
        LogAnalytics(const LogAnalytics&) = delete;
        LogAnalytics& operator=(const LogAnalytics&) = delete;

        void CheckAlertConditions(const LogMessage& message);
        void UpdateMetrics(const LogMessage& message);

      private:
        std::mutex m_mutex;
        LogStatistics m_statistics;
        std::unordered_map<std::string, LogAlertCondition> m_alertConditions;
        std::unordered_map<std::string, LogAlertCallback> m_alertCallbacks;
        MonitoringConfig m_monitoringConfig;
        bool m_isMonitoring{false};

        // Metrics storage
        struct MetricData {
            std::vector<
                std::pair<std::chrono::system_clock::time_point, uint64_t>>
                samples;
            uint64_t totalCount{0};
            double currentRate{0.0};
        };
        std::unordered_map<std::string, MetricData> m_categoryMetrics;
    };

    /**
     * @brief Sink that collects analytics data
     */
    class AnalyticsSink : public ILogSink {
      public:
        void Write(const LogMessage& message) override {
            LogAnalytics::Get().UpdateStatistics(message);
        }

        void Flush() override {}
    };

}  // namespace Engine
