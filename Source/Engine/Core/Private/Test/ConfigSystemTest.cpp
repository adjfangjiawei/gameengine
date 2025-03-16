#include <iostream>

#include "Assert.h"
#include "Config/ConfigSystem.h"
#include "FileSystem/FileSystem.h"

namespace Engine {
    namespace Test {

        void RunConfigSystemTests() {
            ConfigSystem& config = ConfigSystem::Get();

            // Clear any existing configuration
            config.Clear();

            // Test 1: Basic value setting and getting
            {
                // Test boolean value
                config.SetValue("Graphics", "VSync", true);
                bool vsync = config.GetValue<bool>("Graphics", "VSync", false);
                ASSERT(vsync == true, "Failed to get boolean value");

                // Test integer value
                config.SetValue("Graphics", "Resolution_Width", 1920);
                int32_t width =
                    config.GetValue<int32_t>("Graphics", "Resolution_Width", 0);
                ASSERT(width == 1920, "Failed to get integer value");

                // Test float value
                config.SetValue("Audio", "MasterVolume", 0.75f);
                float volume =
                    config.GetValue<float>("Audio", "MasterVolume", 0.0f);
                ASSERT(volume == 0.75f, "Failed to get float value");

                // Test string value
                config.SetValue("Game", "Difficulty", std::string("Normal"));
                std::string difficulty =
                    config.GetValue<std::string>("Game", "Difficulty", "Easy");
                ASSERT(difficulty == "Normal", "Failed to get string value");

                // Test string array value
                std::vector<std::string> languages = {
                    "English", "Spanish", "French"};
                config.SetValue("Game", "SupportedLanguages", languages);
                auto retrievedLangs = config.GetValue<std::vector<std::string>>(
                    "Game", "SupportedLanguages", std::vector<std::string>());
                ASSERT(retrievedLangs == languages,
                       "Failed to get string array value");
            }

            // Test 2: Default values
            {
                bool defaultBool =
                    config.GetValue<bool>("NonExistent", "Key", true);
                ASSERT(defaultBool == true,
                       "Failed to get default boolean value");

                int32_t defaultInt =
                    config.GetValue<int32_t>("NonExistent", "Key", 42);
                ASSERT(defaultInt == 42, "Failed to get default integer value");

                float defaultFloat =
                    config.GetValue<float>("NonExistent", "Key", 3.14f);
                ASSERT(defaultFloat == 3.14f,
                       "Failed to get default float value");

                std::string defaultStr = config.GetValue<std::string>(
                    "NonExistent", "Key", "Default");
                ASSERT(defaultStr == "Default",
                       "Failed to get default string value");
            }

            // Test 3: Key existence and removal
            {
                ASSERT(config.HasKey("Graphics", "VSync"), "Key should exist");
                ASSERT(!config.HasKey("NonExistent", "Key"),
                       "Key should not exist");

                config.RemoveKey("Graphics", "VSync");
                ASSERT(!config.HasKey("Graphics", "VSync"),
                       "Key should have been removed");

                config.RemoveSection("Graphics");
                ASSERT(!config.HasKey("Graphics", "Resolution_Width"),
                       "Section should have been removed");
            }

            // Test 4: Section and key enumeration
            {
                config.Clear();

                // Add some test data
                config.SetValue("Graphics", "VSync", true);
                config.SetValue("Graphics", "Resolution", 1920);
                config.SetValue("Audio", "Volume", 1.0f);
                config.SetValue("Game", "Name", std::string("Test Game"));

                auto sections = config.GetSections();
                ASSERT(sections.size() == 3, "Incorrect number of sections");

                auto graphicsKeys = config.GetKeys("Graphics");
                ASSERT(graphicsKeys.size() == 2,
                       "Incorrect number of keys in Graphics section");
            }

            // Test 5: File operations
            {
                config.Clear();

                // Set up some test configuration
                config.SetValue("Graphics", "VSync", true);
                config.SetValue("Graphics", "Resolution_Width", 1920);
                config.SetValue("Graphics", "Resolution_Height", 1080);
                config.SetValue("Audio", "MasterVolume", 0.75f);
                config.SetValue("Audio", "MusicVolume", 0.8f);
                config.SetValue("Game", "Difficulty", std::string("Normal"));
                config.SetValue(
                    "Game", "PlayerName", std::string("TestPlayer"));

                // Save configuration to file
                const std::string testConfigPath = "test_config.json";
                bool savedSuccess = config.SaveToFile(testConfigPath);
                ASSERT(savedSuccess, "Failed to save configuration file");

                // Clear current configuration
                config.Clear();
                ASSERT(config.GetSections().empty(),
                       "Configuration should be empty after clear");

                // Load configuration from file
                bool loadedSuccess = config.LoadFromFile(testConfigPath);
                ASSERT(loadedSuccess, "Failed to load configuration file");

                // Verify loaded values
                ASSERT(
                    config.GetValue<bool>("Graphics", "VSync", false) == true,
                    "Failed to load boolean value from file");
                ASSERT(config.GetValue<int32_t>(
                           "Graphics", "Resolution_Width", 0) == 1920,
                       "Failed to load integer value from file");
                ASSERT(config.GetValue<float>("Audio", "MasterVolume", 0.0f) ==
                           0.75f,
                       "Failed to load float value from file");
                ASSERT(config.GetValue<std::string>("Game", "PlayerName", "") ==
                           "TestPlayer",
                       "Failed to load string value from file");

                // Clean up test file
                // Engine::FileSystem::DeleteFile(testConfigPath);
            }

            std::cout << "Config system tests completed successfully!"
                      << std::endl;
        }

    }  // namespace Test
}  // namespace Engine

// Add main function
int main() {
    try {
        Engine::Test::RunConfigSystemTests();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
