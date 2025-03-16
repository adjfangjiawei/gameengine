
#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "CoreTypes.h"

namespace Engine {

    /**
     * @brief Represents a configuration value that can be of different types
     */
    using ConfigValue =
        std::variant<bool,                     // Boolean values
                     int32_t,                  // Integer values
                     float,                    // Float values
                     std::string,              // String values
                     std::vector<std::string>  // String array values
                     >;

    /**
     * @brief Configuration system for managing engine and game settings
     *
     * The ConfigSystem provides a centralized way to manage configuration
     * settings. It supports different value types, hierarchical sections, and
     * can load/save configurations from/to files.
     */
    class ConfigSystem {
      public:
        /**
         * @brief Get the singleton instance of the config system
         * @return Reference to the config system instance
         */
        static ConfigSystem &Get();

        /**
         * @brief Load configuration from a file
         * @param filepath Path to the configuration file
         * @return True if loading was successful
         */
        bool LoadFromFile(const std::string &filepath);

        /**
         * @brief Save current configuration to a file
         * @param filepath Path to save the configuration
         * @return True if saving was successful
         */
        bool SaveToFile(const std::string &filepath) const;

        /**
         * @brief Set a configuration value
         * @param section The configuration section
         * @param key The configuration key
         * @param value The value to set
         */
        template <typename T>
        void SetValue(const std::string &section,
                      const std::string &key,
                      const T &value) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_configs[section][key] = value;
        }

        /**
         * @brief Get a configuration value
         * @param section The configuration section
         * @param key The configuration key
         * @param defaultValue The default value to return if key doesn't exist
         * @return The configuration value or default value if not found
         */
        template <typename T>
        T GetValue(const std::string &section,
                   const std::string &key,
                   const T &defaultValue) const {
            std::lock_guard<std::mutex> lock(m_mutex);

            auto sectionIt = m_configs.find(section);
            if (sectionIt != m_configs.end()) {
                auto valueIt = sectionIt->second.find(key);
                if (valueIt != sectionIt->second.end()) {
                    try {
                        return std::get<T>(valueIt->second);
                    } catch (const std::bad_variant_access &) {
                        return defaultValue;
                    }
                }
            }
            return defaultValue;
        }

        /**
         * @brief Check if a configuration key exists
         * @param section The configuration section
         * @param key The configuration key
         * @return True if the key exists
         */
        bool HasKey(const std::string &section, const std::string &key) const;

        /**
         * @brief Remove a configuration key
         * @param section The configuration section
         * @param key The configuration key
         */
        void RemoveKey(const std::string &section, const std::string &key);

        /**
         * @brief Remove an entire configuration section
         * @param section The section to remove
         */
        void RemoveSection(const std::string &section);

        /**
         * @brief Clear all configurations
         */
        void Clear();

        /**
         * @brief Get all sections
         * @return Vector of section names
         */
        std::vector<std::string> GetSections() const;

        /**
         * @brief Get all keys in a section
         * @param section The section name
         * @return Vector of key names
         */
        std::vector<std::string> GetKeys(const std::string &section) const;

      private:
        ConfigSystem() = default;
        ~ConfigSystem() = default;

        ConfigSystem(const ConfigSystem &) = delete;
        ConfigSystem &operator=(const ConfigSystem &) = delete;
        ConfigSystem(ConfigSystem &&) = delete;
        ConfigSystem &operator=(ConfigSystem &&) = delete;

      private:
        using ConfigSection = std::unordered_map<std::string, ConfigValue>;
        std::unordered_map<std::string, ConfigSection> m_configs;
        mutable std::mutex m_mutex;
    };

}  // namespace Engine
