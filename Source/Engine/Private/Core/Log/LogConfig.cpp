
#include "Core/Log/LogConfig.h"

#include <fstream>
#include <functional>
#include <memory>
#include <sstream>

#include "Core/FileSystem/FileSystem.h"
#include "Core/Log/LogSinks.h"
#include "Core/Log/LogSystem.h"
#include "Core/String/StringUtils.h"

namespace Engine {

    namespace {
        // Helper function to parse log level from string
        [[maybe_unused]] LogLevel ParseLogLevel(const std::string& level) {
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
            return LogLevel::Info;  // Default
        }

        // Helper function to convert log level to string
        [[maybe_unused]] std::string LogLevelToString(LogLevel level) {
            switch (level) {
                case LogLevel::Trace:
                    return "trace";
                case LogLevel::Debug:
                    return "debug";
                case LogLevel::Info:
                    return "info";
                case LogLevel::Warning:
                    return "warning";
                case LogLevel::Error:
                    return "error";
                case LogLevel::Fatal:
                    return "fatal";
                default:
                    return "info";
            }
        }

        // Create sink based on configuration
        std::unique_ptr<ILogSink> CreateSink(const LogSinkConfig& config) {
            if (config.type == "console") {
                bool useColors = true;
                auto it = config.parameters.find("use_colors");
                if (it != config.parameters.end()) {
                    useColors = StringUtils::ToBool(it->second);
                }
                auto temp = new ConsoleLogSink(
                    useColors);  // NOLINT(cppcoreguidelines-owning-memory)
                return std::unique_ptr<ILogSink>((ILogSink*)(temp));
            } else if (config.type == "file") {
                std::string filename = "log.txt";
                size_t maxSize = 10 * 1024 * 1024;  // 10MB default
                size_t maxFiles = 5;

                auto filenameIt = config.parameters.find("filename");
                if (filenameIt != config.parameters.end()) {
                    filename = filenameIt->second;
                }

                auto maxSizeIt = config.parameters.find("max_size");
                if (maxSizeIt != config.parameters.end()) {
                    maxSize = std::stoull(maxSizeIt->second);
                }

                auto maxFilesIt = config.parameters.find("max_files");
                if (maxFilesIt != config.parameters.end()) {
                    maxFiles = std::stoull(maxFilesIt->second);
                }

                auto temp = new FileLogSink(
                    filename,
                    true,
                    maxSize,
                    maxFiles);  // NOLINT(cppcoreguidelines-owning-memory)
                return std::unique_ptr<ILogSink>((ILogSink*)(temp));
            } else if (config.type == "rotating") {
                std::string pattern = "log_%Y%m%d_%H%M%S.txt";
                RotatingFileLogSink::RotationInterval interval =
                    RotatingFileLogSink::RotationInterval::Daily;

                auto patternIt = config.parameters.find("pattern");
                if (patternIt != config.parameters.end()) {
                    pattern = patternIt->second;
                }

                auto intervalIt = config.parameters.find("interval");
                if (intervalIt != config.parameters.end()) {
                    if (intervalIt->second == "daily") {
                        interval = RotatingFileLogSink::RotationInterval::Daily;
                    } else if (intervalIt->second == "weekly") {
                        interval =
                            RotatingFileLogSink::RotationInterval::Weekly;
                    } else if (intervalIt->second == "monthly") {
                        interval =
                            RotatingFileLogSink::RotationInterval::Monthly;
                    }
                }

                auto temp = new RotatingFileLogSink(
                    pattern,
                    interval);  // NOLINT(cppcoreguidelines-owning-memory)
                return std::unique_ptr<ILogSink>((ILogSink*)(temp));
            } else if (config.type == "debug") {
                auto temp =
                    new DebugLogSink();  // NOLINT(cppcoreguidelines-owning-memory)
                return std::unique_ptr<ILogSink>((ILogSink*)(temp));
            } else if (config.type == "memory") {
                size_t maxMessages = 1000;
                auto maxMessagesIt = config.parameters.find("max_messages");
                if (maxMessagesIt != config.parameters.end()) {
                    maxMessages = std::stoull(maxMessagesIt->second);
                }
                auto temp = new MemoryLogSink(
                    maxMessages);  // NOLINT(cppcoreguidelines-owning-memory)
                return std::unique_ptr<ILogSink>((ILogSink*)(temp));
            } else if (config.type == "async") {
                auto innerSinkConfig = config.parameters.find("inner_sink");
                if (innerSinkConfig != config.parameters.end()) {
                    LogSinkConfig innerConfig;
                    innerConfig.type = innerSinkConfig->second;
                    // Copy remaining parameters
                    for (const auto& param : config.parameters) {
                        if (param.first != "inner_sink") {
                            innerConfig.parameters[param.first] = param.second;
                        }
                    }
                    auto innerSink = CreateSink(innerConfig);
                    auto temp = new AsyncLogSink(std::move(
                        innerSink));  // NOLINT(cppcoreguidelines-owning-memory)
                    return std::unique_ptr<ILogSink>((ILogSink*)(temp));
                }
            }

            // Default to console sink if type is unknown
            auto temp = new ConsoleLogSink(
                true);  // NOLINT(cppcoreguidelines-owning-memory)
            return std::unique_ptr<ILogSink>((ILogSink*)(temp));
        }
    }  // namespace

    LogConfig LogConfig::LoadFromFile(const std::string& path) {
        std::string extension = StringUtils::GetFileExtension(path);
        std::string content;
        FileSystem::ReadFileToString(path, content);

        if (StringUtils::EqualsIgnoreCase(extension, "json")) {
            return LoadFromJson(content);
        } else if (StringUtils::EqualsIgnoreCase(extension, "xml")) {
            return LoadFromXml(content);
        } else if (StringUtils::EqualsIgnoreCase(extension, "ini")) {
            return LoadFromIni(content);
        }

        throw std::runtime_error("Unsupported configuration file format");
    }

    LogConfig LogConfig::LoadFromJson(
        [[maybe_unused]] const std::string& json) {
        // TODO: Implement JSON parsing
        // For now, return default config
        return LogConfig{};
    }

    LogConfig LogConfig::LoadFromXml([[maybe_unused]] const std::string& xml) {
        // TODO: Implement XML parsing
        // For now, return default config
        return LogConfig{};
    }

    LogConfig LogConfig::LoadFromIni([[maybe_unused]] const std::string& ini) {
        // TODO: Implement INI parsing
        // For now, return default config
        return LogConfig{};
    }

    void LogConfig::SaveToFile(const std::string& path) const {
        std::string extension = StringUtils::GetFileExtension(path);
        std::string content;

        if (StringUtils::EqualsIgnoreCase(extension, "json")) {
            content = ToJson();
        } else if (StringUtils::EqualsIgnoreCase(extension, "xml")) {
            content = ToXml();
        } else if (StringUtils::EqualsIgnoreCase(extension, "ini")) {
            content = ToIni();
        } else {
            throw std::runtime_error("Unsupported configuration file format");
        }

        FileSystem::WriteStringToFile(path, content);
    }

    std::string LogConfig::ToJson() const {
        // TODO: Implement JSON serialization
        return "{}";
    }

    std::string LogConfig::ToXml() const {
        // TODO: Implement XML serialization
        return "<config></config>";
    }

    std::string LogConfig::ToIni() const {
        // TODO: Implement INI serialization
        return "";
    }

    void LogConfig::Apply(LogSystem& system) const {
        // Apply global settings
        system.SetLogLevel(globalLevel);
        system.SetFormat(format);

        // Create and add sinks
        for (const auto& [name, sinkConfig] : sinks) {
            auto sink = CreateSink(sinkConfig);
            system.AddSink(std::move(sink));
        }

        // Apply category-specific configurations
        for (const auto& [category, categoryConfig] : categories) {
            [[maybe_unused]] auto& logger = system.GetLogger(category);
            // TODO: Apply category-specific settings when supported by Logger
            // class
        }
    }

}  // namespace Engine
