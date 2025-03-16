#include <atomic>
#include <chrono>
#include <iostream>
#include <random>

#include "Core/Assert.h"
#include "Core/Threading/ThreadPool.h"

namespace Engine {
    namespace Test {

        // Helper function to simulate work
        void SimulateWork(int milliseconds) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(milliseconds));
        }

        // Helper function to generate random numbers
        int GetRandomNumber(int min, int max) {
            static std::random_device rd;
            static std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(min, max);
            return dis(gen);
        }

        void RunThreadPoolTests() {
            ThreadPool& pool = ThreadPool::Get();

            // Test 1: Basic task submission and execution
            {
                std::atomic<int> counter{0};
                const int numTasks = 100;

                // Initialize pool with 4 threads
                ThreadPoolConfig config;
                config.threadCount = 4;
                pool.Initialize(config);

                // Submit tasks that increment the counter
                std::vector<std::future<void>> futures;
                for (int i = 0; i < numTasks; ++i) {
                    futures.push_back(pool.Submit([&counter]() { counter++; }));
                }

                // Wait for all tasks to complete
                for (auto& future : futures) {
                    future.get();
                }

                ASSERT(counter == numTasks, "Not all tasks were executed");

                pool.Shutdown();
            }

            // Test 2: Tasks with return values
            {
                ThreadPoolConfig config;
                config.threadCount = 4;
                pool.Initialize(config);

                // Submit tasks that return values
                std::vector<std::future<int>> futures;
                for (int i = 0; i < 10; ++i) {
                    futures.push_back(pool.Submit([i]() {
                        SimulateWork(10);
                        return i * i;
                    }));
                }

                // Verify results
                for (int i = 0; i < 10; ++i) {
                    int result = futures[i].get();
                    ASSERT(result == i * i, "Incorrect task result");
                }

                pool.Shutdown();
            }

            // Test 3: Task groups
            {
                ThreadPoolConfig config;
                config.threadCount = 4;
                pool.Initialize(config);

                TaskGroup group;
                std::atomic<int> groupCounter{0};

                // Submit tasks to the group
                for (int i = 0; i < 5; ++i) {
                    group.SubmitTask([&groupCounter]() {
                        SimulateWork(GetRandomNumber(10, 50));
                        groupCounter++;
                    });
                }

                // Wait for all tasks in the group to complete
                group.WaitForCompletion();

                ASSERT(groupCounter == 5, "Not all group tasks completed");

                pool.Shutdown();
            }

            // Test 4: Exception handling
            {
                ThreadPoolConfig config;
                config.threadCount = 2;
                pool.Initialize(config);

                // Submit a task that throws an exception
                auto future = pool.Submit([]() -> int {
                    throw std::runtime_error("Test exception");
                    return 0;
                });

                // Verify that the exception is properly propagated
                bool exceptionCaught = false;
                try {
                    future.get();
                } catch (const std::runtime_error& e) {
                    exceptionCaught = true;
                    ASSERT(std::string(e.what()) == "Test exception",
                           "Incorrect exception message");
                }

                ASSERT(exceptionCaught, "Exception was not propagated");

                pool.Shutdown();
            }

            // Test 5: Performance test with concurrent tasks
            {
                ThreadPoolConfig config;
                const size_t numThreads = std::thread::hardware_concurrency();
                config.threadCount = numThreads;
                pool.Initialize(config);

                const int numTasks = numThreads * 10;
                std::atomic<int> completedTasks{0};

                auto startTime = std::chrono::high_resolution_clock::now();

                // Submit tasks that simulate varying workloads
                std::vector<std::future<void>> futures;
                for (int i = 0; i < numTasks; ++i) {
                    futures.push_back(pool.Submit([&completedTasks]() {
                        SimulateWork(GetRandomNumber(10, 30));
                        completedTasks++;
                    }));
                }

                // Wait for all tasks to complete
                for (auto& future : futures) {
                    future.get();
                }

                auto endTime = std::chrono::high_resolution_clock::now();
                auto duration =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        endTime - startTime);

                ASSERT(completedTasks == numTasks, "Not all tasks completed");

                std::cout << "Performance test completed: " << numTasks
                          << " tasks in " << duration.count() << "ms using "
                          << numThreads << " threads" << std::endl;

                pool.Shutdown();
            }

            // Test 6: Stress test with many small tasks
            {
                ThreadPoolConfig config;
                config.threadCount = 4;
                pool.Initialize(config);

                const int numTasks = 1000;
                std::atomic<int> sum{0};

                // Submit many small tasks
                std::vector<std::future<void>> futures;
                for (int i = 0; i < numTasks; ++i) {
                    futures.push_back(pool.Submit([&sum]() { sum++; }));
                }

                // Wait for all tasks to complete
                for (auto& future : futures) {
                    future.get();
                }

                ASSERT(sum == numTasks,
                       "Not all tasks were executed in stress test");

                pool.Shutdown();
            }

            // Test 7: Verify shutdown behavior
            {
                ThreadPoolConfig config;
                config.threadCount = 2;
                pool.Initialize(config);

                // Submit some tasks
                std::vector<std::future<void>> futures;
                for (int i = 0; i < 5; ++i) {
                    futures.push_back(pool.Submit([]() { SimulateWork(50); }));
                }

                // Shutdown should wait for all tasks to complete
                pool.Shutdown();

                // Verify all tasks completed
                bool allTasksCompleted = true;
                for (auto& future : futures) {
                    allTasksCompleted &= future.valid();
                }

                ASSERT(allTasksCompleted,
                       "Not all tasks completed before shutdown");
            }

            std::cout << "Thread pool tests completed successfully!"
                      << std::endl;
        }

    }  // namespace Test
}  // namespace Engine
