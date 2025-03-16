#include "Core/Config/ConfigSystem.h"

#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>

#include "Core/Assert.h"
#include "Core/FileSystem/FileSystem.h"
#include "Core/Log/LogSystem.h"

namespace Engine {

    using json = nlohmann::json;

    namespace {
        // Convert ConfigValue to JSON
        json ConfigValueToJson(const ConfigValue &value) {
            return std::visit([](const auto &v) -> json { return v; }, value);
        }

        // Convert JSON to ConfigValue
        ConfigValue JsonToConfigValue(const json &j) {
            if (j.is_boolean()) {
                return j.get<bool>();
            } else if (j.is_number_integer()) {
                return j.get<int32_t>();
            } else if (j.is_number_float()) {
                return j.get<float>();
            } else if (j.is_string()) {
                return j.get<std::string>();
            } else if (j.is_array()) {
                std::vector<std::string> strArray;
                for (const auto &element : j) {
                    if (element.is_string()) {
                        strArray.push_back(element.get<std::string>());
                    }
                }
                return strArray;
            }

            // Default to empty string if type is not supported
            return std::string();
        }
    }  // namespace

    ConfigSystem &ConfigSystem::Get() {
        static ConfigSystem instance;
        return instance;
    }

    bool ConfigSystem::LoadFromFile(const std::string &filepath) {
        try {
            // Read file content
            std::string content;
            if (!FileSystem::ReadFileToString(filepath, content)) {
                LOG_ERROR("Failed to read configuration file: {}", filepath);
                return false;
            }

            // Parse JSON
            json j = json::parse(content);

            std::lock_guard<std::mutex> lock(m_mutex);
            m_configs.clear();

            // Convert JSON to internal format
            for (auto it = j.begin(); it != j.end(); ++it) {
                const std::string &section = it.key();
                const json &sectionData = it.value();

                if (sectionData.is_object()) {
                    for (auto valueIt = sectionData.begin();
                         valueIt != sectionData.end();
                         ++valueIt) {
                        const std::string &key = valueIt.key();
                        m_configs[section][key] =
                            JsonToConfigValue(valueIt.value());
                    }
                }
            }

            return true;
        } catch (const std::exception &e) {
            LOG_ERROR("Error loading configuration file: {}", e.what());
            return false;
        }
    }

    bool ConfigSystem::SaveToFile(const std::string &filepath) const {
        try {
            json j;

            // Convert internal format to JSON
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                for (const auto &section : m_configs) {
                    json sectionJson;
                    for (const auto &value : section.second) {
                        sectionJson[value.first] =
                            ConfigValueToJson(value.second);
                    }
                    j[section.first] = sectionJson;
                }
            }

            // Convert to string with pretty printing
            std::string content = j.dump(4);

            // Write to file
            if (!FileSystem::WriteStringToFile(filepath, content)) {
                LOG_ERROR("Failed to write configuration file: {}", filepath);
                return false;
            }

            return true;
        } catch (const std::exception &e) {
            LOG_ERROR("Error saving configuration file: {}", e.what());
            return false;
        }
    }

    bool ConfigSystem::HasKey(const std::string &section,
                              const std::string &key) const {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto sectionIt = m_configs.find(section);
        if (sectionIt != m_configs.end()) {
            return sectionIt->second.find(key) != sectionIt->second.end();
        }
        return false;
    }

    void ConfigSystem::RemoveKey(const std::string &section,
                                 const std::string &key) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto sectionIt = m_configs.find(section);
        if (sectionIt != m_configs.end()) {
            sectionIt->second.erase(key);

            // Remove section if empty
            if (sectionIt->second.empty()) {
                m_configs.erase(sectionIt);
            }
        }
    }

    void ConfigSystem::RemoveSection(const std::string &section) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_configs.erase(section);
    }

    void ConfigSystem::Clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_configs.clear();
    }

    std::vector<std::string> ConfigSystem::GetSections() const {
        std::vector<std::string> sections;

        std::lock_guard<std::mutex> lock(m_mutex);
        sections.reserve(m_configs.size());

        for (const auto &section : m_configs) {
            sections.push_back(section.first);
        }

        return sections;
    }

    std::vector<std::string> ConfigSystem::GetKeys(
        const std::string &section) const {
        std::vector<std::string> keys;

        std::lock_guard<std::mutex> lock(m_mutex);

        auto sectionIt = m_configs.find(section);
        if (sectionIt != m_configs.end()) {
            keys.reserve(sectionIt->second.size());
            for (const auto &key : sectionIt->second) {
                keys.push_back(key.first);
            }
        }

        return keys;
    }

}  // namespace Engine
