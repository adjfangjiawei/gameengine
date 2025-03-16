
#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "Log/LogSystem.h"

namespace Engine {

    /**
     * @brief Configuration for a log sink
     */
    struct LogSinkConfig {
        std::string
            type;  // "console", "file", "rotating", "debug", "memory", "async"
        std::unordered_map<std::string, std::string> parameters;
    };

    /**
     * @brief Configuration for category-specific settings
     */
    struct LogCategoryConfig {
        LogLevel level = LogLevel::Info;
        std::vector<std::string> sinks;
        bool enabled = true;
    };

    /**
     * @brief Main configuration structure for the logging system
     */
    class LogConfig {
      public:
        // Global settings
        LogLevel globalLevel = LogLevel::Info;
        LogFormat format;

        // Sink configurations
        std::unordered_map<std::string, LogSinkConfig> sinks;

        // Category-specific configurations
        std::unordered_map<std::string, LogCategoryConfig> categories;

        // Configuration loading methods
        static LogConfig LoadFromFile(const std::string& path);
        static LogConfig LoadFromJson(const std::string& json);
        static LogConfig LoadFromXml(const std::string& xml);
        static LogConfig LoadFromIni(const std::string& ini);

        // Configuration saving methods
        void SaveToFile(const std::string& path) const;
        std::string ToJson() const;
        std::string ToXml() const;
        std::string ToIni() const;

        // Apply configuration to the logging system
        void Apply(LogSystem& system) const;
    };

}  // namespace Engine
