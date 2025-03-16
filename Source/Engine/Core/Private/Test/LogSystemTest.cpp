
#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>

#include "Assert.h"
#include "Log/LogSinks.h"
#include "Log/LogSystem.h"

namespace Engine {
    namespace Test {

        void RunLogSystemTests() {
            std::cout << "\nRunning Log System Tests:" << std::endl;

            // Test 1: Basic Logging
            {
                std::cout << "\nTesting Basic Logging:" << std::endl;

                LogSystem::Get().Initialize();

                // Add console sink
                auto consoleSink = std::make_unique<ConsoleLogSink>(true);
                LogSystem::Get().AddSink(std::move(consoleSink));

                // Test different log levels
                LOG_TRACE("Test", "This is a trace message");
                LOG_DEBUG("Test", "This is a debug message");
                LOG_INFO("Test", "This is an info message");
                LOG_WARNING("Test", "This is a warning message");
                LOG_ERROR("Test", "This is an error message");
                LOG_FATAL("Test", "This is a fatal message");

                std::cout << "Basic logging tests completed!" << std::endl;
            }

            // Test 2: File Logging
            {
                std::cout << "\nTesting File Logging:" << std::endl;

                const std::string logFile = "test.log";
                auto fileSink = std::make_unique<FileLogSink>(logFile);
                LogSystem::Get().AddSink(std::move(fileSink));

                LOG_INFO("FileTest", "Testing file logging");
                LOG_ERROR("FileTest", "Test error message to file");

                // Verify file exists and has content
                ASSERT(std::filesystem::exists(logFile),
                       "Log file was not created");
                ASSERT(std::filesystem::file_size(logFile) > 0,
                       "Log file is empty");

                // Clean up
                std::filesystem::remove(logFile);

                std::cout << "File logging tests completed!" << std::endl;
            }

            // Test 3: Memory Logging
            {
                std::cout << "\nTesting Memory Logging:" << std::endl;

                auto memorySink = std::make_unique<MemoryLogSink>(100);
                auto *memSinkPtr = memorySink.get();
                LogSystem::Get().AddSink(std::move(memorySink));

                LOG_INFO("MemTest", "Memory log message 1");
                LOG_INFO("MemTest", "Memory log message 2");

                auto messages = memSinkPtr->GetMessages();
                ASSERT(messages.size() == 2,
                       "Incorrect number of messages in memory sink");
                ASSERT(messages[0].message.find("Memory log message 1") !=
                           std::string::npos,
                       "First message content incorrect");

                std::cout << "Memory logging tests completed!" << std::endl;
            }

            // Test 4: Async Logging
            {
                std::cout << "\nTesting Async Logging:" << std::endl;

                auto memorySink = std::make_unique<MemoryLogSink>(100);
                auto asyncSink =
                    std::make_unique<AsyncLogSink>(std::move(memorySink));
                LogSystem::Get().AddSink(std::move(asyncSink));

                // Log messages from multiple threads
                std::vector<std::thread> threads;
                for (int i = 0; i < 5; ++i) {
                    threads.emplace_back([i]() {
                        for (int j = 0; j < 10; ++j) {
                            LOG_INFO("AsyncTest", "Thread {} message {}", i, j);
                        }
                    });
                }

                for (auto &thread : threads) {
                    thread.join();
                }

                // Allow time for async processing
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                std::cout << "Async logging tests completed!" << std::endl;
            }

            // Test 5: Rotating File Log
            {
                std::cout << "\nTesting Rotating File Log:" << std::endl;

                auto rotatingSink = std::make_unique<RotatingFileLogSink>(
                    "test_log_%Y%m%d.log",
                    RotatingFileLogSink::RotationInterval::Daily);
                LogSystem::Get().AddSink(std::move(rotatingSink));

                LOG_INFO("RotateTest", "Testing rotating file log");

                // Clean up test files
                for (const auto &entry :
                     std::filesystem::directory_iterator(".")) {
                    if (entry.path().string().find("test_log_") !=
                        std::string::npos) {
                        std::filesystem::remove(entry.path());
                    }
                }

                std::cout << "Rotating file log tests completed!" << std::endl;
            }

            // Test 6: Log Formatting
            {
                std::cout << "\nTesting Log Formatting:" << std::endl;

                LogFormat format;
                format.showTimestamp = true;
                format.showLevel = true;
                format.showCategory = true;
                format.showThreadId = true;
                format.showLocation = true;
                format.messageFormat =
                    "[{timestamp}][{level}][{category}] {message}";

                LogSystem::Get().SetFormat(format);

                LOG_INFO("FormatTest", "Testing custom format");

                std::cout << "Log formatting tests completed!" << std::endl;
            }

            // Test 7: Debug Log Sink
            {
                std::cout << "\nTesting Debug Log Sink:" << std::endl;

                auto debugSink = std::make_unique<DebugLogSink>();
                LogSystem::Get().AddSink(std::move(debugSink));

                LOG_DEBUG("DebugTest", "Testing debug output");
                LOG_ERROR("DebugTest", "Testing debug error output");

                std::cout << "Debug log sink tests completed!" << std::endl;
            }

            // Test 8: Log Level Filtering
            {
                std::cout << "\nTesting Log Level Filtering:" << std::endl;

                auto memorySink = std::make_unique<MemoryLogSink>(100);
                auto *memSinkPtr = memorySink.get();
                LogSystem::Get().AddSink(std::move(memorySink));

                LogSystem::Get().SetLogLevel(LogLevel::Warning);

                LOG_DEBUG("FilterTest", "This should not appear");
                LOG_INFO("FilterTest", "This should not appear");
                LOG_WARNING("FilterTest", "This should appear");
                LOG_ERROR("FilterTest", "This should appear");

                auto messages = memSinkPtr->GetMessages();
                ASSERT(messages.size() == 2, "Log level filtering failed");

                std::cout << "Log level filtering tests completed!"
                          << std::endl;
            }

            // Test 9: Multiple Categories
            {
                std::cout << "\nTesting Multiple Categories:" << std::endl;

                auto &logger1 = LogSystem::Get().GetLogger("Category1");
                auto &logger2 = LogSystem::Get().GetLogger("Category2");

                logger1.Info("Message from category 1");
                logger2.Info("Message from category 2");

                std::cout << "Multiple categories tests completed!"
                          << std::endl;
            }

            // Clean up
            LogSystem::Get().Shutdown();

            std::cout << "\nAll log system tests completed successfully!"
                      << std::endl;
        }

    }  // namespace Test
}  // namespace Engine
