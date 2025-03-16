
#include "Core/Log/LogAnalytics.h"

#include <algorithm>
#include <numeric>
#include <regex>
#include <sstream>

namespace Engine {

    namespace {
        // Constants for analytics
        constexpr double SPIKE_THRESHOLD = 2.0;  // 2x normal rate

        // Helper function to calculate message rate
        [[maybe_unused]] double CalculateRate(
            uint64_t count, const LogAnalytics::TimeRange& range) {
            auto duration = range.end - range.start;
            auto seconds = std::chrono::duration<double>(duration).count();
            return static_cast<double>(count) / seconds;
        }
    }  // namespace

    LogAnalytics& LogAnalytics::Get() {
        static LogAnalytics instance;
        return instance;
    }

    void LogAnalytics::UpdateStatistics(const LogMessage& message) {
        std::lock_guard<std::mutex> lock(this->m_mutex);

        // Update global statistics
        m_statistics.totalMessages++;
        if (m_statistics.startTime == std::chrono::system_clock::time_point{}) {
            m_statistics.startTime = message.timestamp;
        }
        m_statistics.lastUpdate = message.timestamp;

        // Update category statistics
        auto& categoryStats = m_statistics.categoryStats[message.category];
        categoryStats.messageCount++;
        categoryStats.levelCounts[message.level]++;
        categoryStats.lastMessage = message.timestamp;

        // Calculate messages per second for the category
        auto duration = message.timestamp - m_statistics.startTime;
        auto seconds = std::chrono::duration<double>(duration).count();
        categoryStats.messagesPerSecond =
            static_cast<double>(categoryStats.messageCount) / seconds;

        // Check alert conditions
        CheckAlertConditions(message);

        // Update metrics if monitoring is enabled
        if (m_isMonitoring) {
            UpdateMetrics(message);
        }
    }

    LogStatistics LogAnalytics::GetStatistics() {
        std::lock_guard<std::mutex> lock(this->m_mutex);
        return m_statistics;
    }

    void LogAnalytics::ResetStatistics() {
        std::lock_guard<std::mutex> lock(this->m_mutex);
        m_statistics = LogStatistics{};
    }

    void LogAnalytics::AddAlertCondition(const std::string& name,
                                         const LogAlertCondition& condition,
                                         LogAlertCallback callback) {
        std::lock_guard<std::mutex> lock(this->m_mutex);
        m_alertConditions[name] = condition;
        m_alertCallbacks[name] = callback;
    }

    void LogAnalytics::RemoveAlertCondition(const std::string& name) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_alertConditions.erase(name);
        m_alertCallbacks.erase(name);
    }

    void LogAnalytics::EnableAlertCondition(const std::string& name,
                                            bool enable) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_alertConditions.find(name);
        if (it != m_alertConditions.end()) {
            it->second.enabled = enable;
        }
    }

    std::vector<LogMessage> LogAnalytics::QueryLogs(
        [[maybe_unused]] const TimeRange& range,
        [[maybe_unused]] const std::string& category,
        [[maybe_unused]] LogLevel minLevel,
        const std::string& pattern) {
        std::vector<LogMessage> results;
        std::regex patternRegex(pattern.empty() ? ".*" : pattern);

        // Note: This assumes we have access to stored log messages
        // In a real implementation, this would query a log storage system

        return results;
    }

    LogAnalytics::LogReport LogAnalytics::GenerateReport(
        [[maybe_unused]] const TimeRange& range, const std::string& category) {
        std::lock_guard<std::mutex> lock(m_mutex);
        LogReport report;
        report.category = category;

        auto it = m_statistics.categoryStats.find(category);
        if (it != m_statistics.categoryStats.end()) {
            const auto& stats = it->second;
            report.messageCount = stats.messageCount;
            report.levelDistribution = stats.levelCounts;
            report.averageRate = stats.messagesPerSecond;

            // Find activity spikes
            auto& metricData = m_categoryMetrics[category];
            double avgRate = report.averageRate;

            for (size_t i = 1; i < metricData.samples.size(); ++i) {
                const auto& current = metricData.samples[i];
                const auto& prev = metricData.samples[i - 1];

                double currentRate =
                    static_cast<double>(current.second - prev.second) /
                    std::chrono::duration<double>(current.first - prev.first)
                        .count();

                if (currentRate > avgRate * SPIKE_THRESHOLD) {
                    TimeRange spikeRange{prev.first, current.first};
                    report.activitySpikes.push_back(spikeRange);
                }
            }
        }

        return report;
    }

    void LogAnalytics::ConfigureMonitoring(const MonitoringConfig& config) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_monitoringConfig = config;
    }

    void LogAnalytics::StartMonitoring() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_isMonitoring = true;
    }

    void LogAnalytics::StopMonitoring() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_isMonitoring = false;
    }

    void LogAnalytics::CheckAlertConditions(const LogMessage& message) {
        for (const auto& [name, condition] : m_alertConditions) {
            if (!condition.enabled) continue;

            bool shouldAlert = false;
            std::string alertMessage;

            switch (condition.type) {
                case LogAlertCondition::Type::MessageRate:
                    {
                        auto& metrics = m_categoryMetrics[message.category];
                        if (metrics.currentRate > condition.threshold) {
                            shouldAlert = true;
                            alertMessage = StringUtils::Format(
                                "Message rate exceeded threshold: {:.2f} > "
                                "{:.2f} msg/s",
                                metrics.currentRate,
                                condition.threshold);
                        }
                        break;
                    }
                case LogAlertCondition::Type::ErrorCount:
                    {
                        if (message.level >= condition.level) {
                            auto& stats =
                                m_statistics.categoryStats[message.category];
                            auto errorCount = stats.levelCounts[message.level];
                            if (errorCount > condition.threshold) {
                                shouldAlert = true;
                                alertMessage = StringUtils::Format(
                                    "Error count exceeded threshold: {} > {}",
                                    errorCount,
                                    static_cast<int>(condition.threshold));
                            }
                        }
                        break;
                    }
                case LogAlertCondition::Type::PatternMatch:
                    {
                        if (std::regex_search(message.message,
                                              std::regex(condition.pattern))) {
                            shouldAlert = true;
                            alertMessage =
                                "Pattern match detected: " + condition.pattern;
                        }
                        break;
                    }
                case LogAlertCondition::Type::CategoryActivity:
                    {
                        if (message.category == condition.category) {
                            auto& stats =
                                m_statistics.categoryStats[message.category];
                            if (stats.messagesPerSecond > condition.threshold) {
                                shouldAlert = true;
                                alertMessage = StringUtils::Format(
                                    "Category activity exceeded threshold: "
                                    "{:.2f} > {:.2f} msg/s",
                                    stats.messagesPerSecond,
                                    condition.threshold);
                            }
                        }
                        break;
                    }
            }

            if (shouldAlert) {
                auto callback = m_alertCallbacks.find(name);
                if (callback != m_alertCallbacks.end()) {
                    callback->second(alertMessage, message);
                }
            }
        }
    }

    void LogAnalytics::UpdateMetrics(const LogMessage& message) {
        if (!m_monitoringConfig.enableRateMonitoring) return;

        auto now = std::chrono::system_clock::now();
        auto& metrics = m_categoryMetrics[message.category];

        // Add new sample
        metrics.samples.emplace_back(now, ++metrics.totalCount);

        // Remove old samples
        auto windowStart = now - m_monitoringConfig.aggregationWindow;
        metrics.samples.erase(std::remove_if(metrics.samples.begin(),
                                             metrics.samples.end(),
                                             [windowStart](const auto& sample) {
                                                 return sample.first <
                                                        windowStart;
                                             }),
                              metrics.samples.end());

        // Calculate current rate
        if (metrics.samples.size() >= 2) {
            const auto& newest = metrics.samples.back();
            const auto& oldest = metrics.samples.front();

            auto duration = newest.first - oldest.first;
            auto count = newest.second - oldest.second;

            metrics.currentRate =
                static_cast<double>(count) /
                std::chrono::duration<double>(duration).count();
        }
    }

}  // namespace Engine
