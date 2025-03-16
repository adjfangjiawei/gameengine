
#include <atomic>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "Assert.h"
#include "Memory/ObjectPool.h"
#include "Threading/ThreadPool.h"

namespace Engine {
    namespace Test {

        // Test class for object pool
        class PooledObject {
          public:
            PooledObject() : m_id(s_nextId++) {}

            void SetValue(int value) { m_value = value; }
            int GetValue() const { return m_value; }
            int GetId() const { return m_id; }

            static void ResetIdCounter() { s_nextId = 0; }

          private:
            int m_value = 0;
            int m_id = 0;
            static std::atomic<int> s_nextId;
        };

        std::atomic<int> PooledObject::s_nextId(0);

        void RunObjectPoolTests() {
            // Reset the ID counter before starting tests
            PooledObject::ResetIdCounter();

            // Test 1: Basic object acquisition and return
            {
                ObjectPool<PooledObject> pool(5, 5);

                // Verify initial pool size
                ASSERT(pool.GetTotalSize() == 5, "Incorrect initial pool size");
                ASSERT(pool.GetAvailableCount() == 5,
                       "Incorrect initial available count");

                // Acquire objects
                auto obj1 = pool.Acquire();
                auto obj2 = pool.Acquire();

                ASSERT(pool.GetAvailableCount() == 3,
                       "Incorrect available count after acquisition");

                // Verify objects are different
                ASSERT(obj1->GetId() != obj2->GetId(),
                       "Objects should have different IDs");

                // Return objects (happens automatically when unique_ptr is
                // destroyed)
                obj1.reset();
                obj2.reset();

                ASSERT(pool.GetAvailableCount() == 5,
                       "Incorrect available count after return");
            }

            // Test 2: Pool growth
            {
                ObjectPool<PooledObject> pool(2, 2, 6);

                std::vector<
                    std::unique_ptr<PooledObject,
                                    std::function<void(PooledObject *)>>>
                    objects;

                // Acquire more objects than initial size
                for (int i = 0; i < 5; ++i) {
                    objects.push_back(pool.Acquire());
                }

                ASSERT(pool.GetTotalSize() == 6, "Pool didn't grow correctly");

                // Try to acquire more than max size
                auto extraObj = pool.Acquire();
                ASSERT(extraObj == nullptr, "Pool exceeded maximum size");

                // Return all objects
                objects.clear();

                ASSERT(pool.GetAvailableCount() == 6,
                       "Not all objects returned to pool");
            }

            // Test 3: Object initialization and reset
            {
                ObjectPool<PooledObject> pool(3, 3);

                // Set init and reset functions
                pool.SetInitFunction(
                    [](PooledObject *obj) { obj->SetValue(42); });

                pool.SetResetFunction(
                    [](PooledObject *obj) { obj->SetValue(0); });

                // Acquire and verify initialization
                auto obj = pool.Acquire();
                ASSERT(obj->GetValue() == 42,
                       "Object not properly initialized");

                // Modify object
                obj->SetValue(100);

                // Return object
                obj.reset();

                // Acquire same object and verify reset
                obj = pool.Acquire();
                ASSERT(obj->GetValue() == 42, "Object not properly reset");
            }

            // Test 4: Multi-threaded usage
            {
                const int numThreads = 4;
                const int numIterations = 1000;

                ObjectPool<PooledObject> pool(32, 32, 128);
                std::atomic<int> successfulAcquisitions{0};

                ThreadPoolConfig config;
                config.threadCount = numThreads;
                ThreadPool::Get().Initialize(config);

                std::vector<std::future<void>> futures;

                // Submit tasks that acquire and release objects
                for (int i = 0; i < numThreads; ++i) {
                    futures.push_back(ThreadPool::Get().Submit([&]() {
                        for (int j = 0; j < numIterations; ++j) {
                            auto obj = pool.Acquire();
                            if (obj) {
                                successfulAcquisitions++;
                                // Simulate some work
                                std::this_thread::sleep_for(
                                    std::chrono::microseconds(10));
                            }
                        }
                    }));
                }

                // Wait for all tasks to complete
                for (auto &future : futures) {
                    future.get();
                }

                ThreadPool::Get().Shutdown();

                ASSERT(
                    pool.GetAvailableCount() == pool.GetTotalSize(),
                    "Not all objects returned to pool in multi-threaded test");
            }

            // Test 5: Object Pool Manager
            {
                auto &manager = ObjectPoolManager::Get();

                // Get pools for different types
                auto &intPool = manager.GetPool<int>(10, 5);
                auto &stringPool = manager.GetPool<std::string>(5, 5);

                // Verify pools are working
                auto intObj = intPool.Acquire();
                auto strObj = stringPool.Acquire();

                ASSERT(intPool.GetAvailableCount() == 9,
                       "Int pool count incorrect");
                ASSERT(stringPool.GetAvailableCount() == 4,
                       "String pool count incorrect");

                // Get same pool again and verify it's the same instance
                auto &intPool2 = manager.GetPool<int>();
                ASSERT(&intPool == &intPool2,
                       "Pool manager returned different instance");
            }

            // Test 6: Performance comparison
            {
                const int numObjects = 10000;

                // Time regular allocation/deallocation
                auto startTime = std::chrono::high_resolution_clock::now();

                for (int i = 0; i < numObjects; ++i) {
                    auto obj = std::make_unique<PooledObject>();
                    obj->SetValue(i);
                }

                auto regularTime =
                    std::chrono::duration_cast<std::chrono::microseconds>(
                        std::chrono::high_resolution_clock::now() - startTime);

                // Time object pool allocation/deallocation
                ObjectPool<PooledObject> pool(1000, 1000);

                startTime = std::chrono::high_resolution_clock::now();

                for (int i = 0; i < numObjects; ++i) {
                    auto obj = pool.Acquire();
                    obj->SetValue(i);
                }

                auto poolTime =
                    std::chrono::duration_cast<std::chrono::microseconds>(
                        std::chrono::high_resolution_clock::now() - startTime);

                std::cout << "Performance comparison:" << std::endl
                          << "Regular allocation: " << regularTime.count()
                          << " microseconds" << std::endl
                          << "Pool allocation: " << poolTime.count()
                          << " microseconds" << std::endl;
            }

            std::cout << "Object pool tests completed successfully!"
                      << std::endl;
        }

    }  // namespace Test
}  // namespace Engine
