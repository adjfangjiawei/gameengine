#include <iostream>
#include <thread>
#include <vector>

#include "Assert.h"
#include "Log/LogConfig.h"
#include "Log/LogSystem.h"
#include "Platform.h"
#include "CoreTypes.h"

using namespace Engine;

// Test helper macros
#define TEST_CASE(name)                                        \
    std::cout << "\nRunning test case: " << name << std::endl; \
    LOG_INFO("Starting test case: {}", name)

#define TEST_ASSERT(condition, message)            \
    do {                                           \
        if (!(condition)) {                        \
            LOG_ERROR("Test failed: {}", message); \
            return 1;                              \
        }                                          \
    } while (0)

// Test platform detection
int TestPlatform() {
    TEST_CASE("Platform Detection");

    LOG_INFO("Platform: {}", Platform::GetPlatformName());
    LOG_INFO("Architecture: {}", Platform::GetArchitectureName());
    LOG_INFO("Compiler: {}", Platform::GetCompilerName());

    return 0;
}

// Test logging system
int TestLogging() {
    TEST_CASE("Logging System");

    // Test basic logging
    LOG_DEBUG("Debug message test");
    LOG_INFO("Info message test");
    LOG_WARNING("Warning message test");
    LOG_ERROR("Error message test");

    // Test formatted logging
    LOG_INFO("Formatted message: {} {} {}", 1, "test", 3.14f);

    // Test file logging
    LogConfig config;
    LogSinkConfig fileSink;
    fileSink.type = "file";
    fileSink.parameters["path"] = "test.log";
    config.sinks["file"] = fileSink;

    // Configure default category to use file sink
    LogCategoryConfig defaultCategory;
    defaultCategory.sinks.push_back("file");
    config.categories["Default"] = defaultCategory;

    // Apply configuration
    config.Apply(LogSystem::Get());

    LOG_INFO("This message should go to both console and file");

    // Test logging from multiple threads
    std::vector<std::thread> threads;
    for (int i = 0; i < 3; ++i) {
        threads.emplace_back([i] { LOG_INFO("Message from thread {}", i); });
        threads.emplace_back([i] { LOG_INFO("Message from thread {}", i); });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    return 0;
}

// Test assertion system
int TestAssertions() {
    TEST_CASE("Assertion System");

    // Test soft assert (should not break)
    ASSERT_SOFT(1 + 1 == 2, "Basic arithmetic check");
    ASSERT_SOFT(true, "Basic boolean check");

    // Test regular assert
    ASSERT(1 + 1 == 2, "Basic arithmetic check");
    ASSERT(true, "Basic boolean check");

    // Test fatal assert
    ASSERT_MSG(1 + 1 == 2, "Fatal arithmetic check", Engine::AssertType::Fatal);
    ASSERT_MSG(true, "Fatal boolean check", Engine::AssertType::Fatal);

    return 0;
}

int main() {
    LOG_INFO("Starting core system tests...");
#ifdef BUILD_DEBUG
    LOG_INFO("Build Type: Debug");
#else
    LOG_INFO("Build Type: Release");
#endif

    int result = 0;

    // Run platform tests
    result |= TestPlatform();

    // Run logging tests
    result |= TestLogging();

    // Run assertion tests
    result |= TestAssertions();

    if (result == 0) {
        LOG_INFO("All tests passed successfully!");
    } else {
        LOG_ERROR("Some tests failed!");
    }

    return result;
}
