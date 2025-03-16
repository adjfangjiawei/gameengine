
#include <gtest/gtest.h>

#include <chrono>
#include <random>
#include <thread>
#include <vector>

#include "Memory/HeapAllocator.h"

using namespace Engine::Memory;

class HeapAllocatorTest : public ::testing::Test {
  protected:
    HeapAllocator::Config config;
    std::unique_ptr<HeapAllocator> allocator;

    void SetUp() override {
        config.trackAllocations = true;
        config.enableDebugInfo = true;
        config.enableGuardBytes = true;
        config.enableDefragmentation = true;
        config.defragmentationThreshold = 75;
        config.guardSize = 8;
        config.debugBackTraceDepth = 32;
        allocator = std::make_unique<HeapAllocator>(config);
    }

    void TearDown() override { allocator.reset(); }
};

TEST_F(HeapAllocatorTest, BasicAllocation) {
    // Test basic allocation
    void *ptr = allocator->Allocate(100);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(allocator->GetAllocationSize(ptr), 100);

    // Verify memory stats
    const auto &stats = allocator->GetDetailedStats();
    EXPECT_EQ(stats.currentUsage, 100);
    EXPECT_EQ(stats.allocationCount, 1);
    EXPECT_GT(stats.headerOverhead, 0);
    EXPECT_GT(stats.guardOverhead, 0);

    allocator->Deallocate(ptr);
    EXPECT_EQ(stats.currentUsage, 0);
}

TEST_F(HeapAllocatorTest, AlignmentTest) {
    // Test different alignments
    std::vector<size_t> alignments = {8, 16, 32, 64, 128};
    std::vector<void *> ptrs;

    for (size_t alignment : alignments) {
        void *ptr = allocator->Allocate(100, alignment);
        ASSERT_NE(ptr, nullptr);
        EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % alignment, 0);
        ptrs.push_back(ptr);
    }

    // Cleanup
    for (void *ptr : ptrs) {
        allocator->Deallocate(ptr);
    }
}

TEST_F(HeapAllocatorTest, TypeTracking) {
    // Test allocation tracking by type
    void *ptr1 = allocator->AllocateTracked(100, 8, "TestType1");
    void *ptr2 = allocator->AllocateTracked(200, 8, "TestType2");
    void *ptr3 = allocator->AllocateTracked(300, 8, "TestType1");

    const auto &stats = allocator->GetDetailedStats();
    auto type1It = stats.allocationsByType.find("TestType1");
    auto type2It = stats.allocationsByType.find("TestType2");
    ASSERT_NE(type1It, stats.allocationsByType.end());
    ASSERT_NE(type2It, stats.allocationsByType.end());
    EXPECT_EQ(type1It->second, 400);
    EXPECT_EQ(type2It->second, 200);

    allocator->Deallocate(ptr1);
    type1It = stats.allocationsByType.find("TestType1");
    ASSERT_NE(type1It, stats.allocationsByType.end());
    EXPECT_EQ(type1It->second, 300);

    allocator->Deallocate(ptr2);
    allocator->Deallocate(ptr3);
}

TEST_F(HeapAllocatorTest, GuardBytesProtection) {
    // Test memory guard protection
    void *ptr = allocator->Allocate(100);
    ASSERT_NE(ptr, nullptr);

    // Writing within bounds should be fine
    char *charPtr = static_cast<char *>(ptr);
    for (size_t i = 0; i < 100; ++i) {
        charPtr[i] = static_cast<char>(i);
    }

    // Writing beyond bounds should trigger assertion
    EXPECT_DEATH({ charPtr[100] = 0xFF; }, "Memory corruption detected");

    allocator->Deallocate(ptr);
}

TEST_F(HeapAllocatorTest, ConcurrentAccess) {
    // Test thread safety with read-write lock
    constexpr int numThreads = 8;
    constexpr int numOperations = 1000;
    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};

    auto threadFunc = [this, &successCount](int threadId) {
        std::vector<void *> ptrs;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> sizeDist(1, 1000);
        std::uniform_int_distribution<> alignDist(8, 128);

        for (int i = 0; i < numOperations; ++i) {
            if (i % 2 == 0) {
                // Allocate
                size_t size = sizeDist(gen);
                size_t alignment = alignDist(gen);
                void *ptr = allocator->AllocateTracked(
                    size,
                    alignment,
                    ("Type" + std::to_string(threadId)).c_str());
                if (ptr) {
                    ptrs.push_back(ptr);
                    successCount++;
                }
            } else if (!ptrs.empty()) {
                // Deallocate
                size_t index = gen() % ptrs.size();
                allocator->Deallocate(ptrs[index]);
                ptrs[index] = ptrs.back();
                ptrs.pop_back();
            }

            // Occasionally check stats
            if (i % 100 == 0) {
                const auto &stats = allocator->GetDetailedStats();
                EXPECT_GE(stats.totalAllocated, stats.currentUsage);
            }
        }

        // Cleanup remaining allocations
        for (void *ptr : ptrs) {
            allocator->Deallocate(ptr);
        }
    };

    // Start threads
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(threadFunc, i);
    }

    // Join threads
    for (auto &thread : threads) {
        thread.join();
    }

    // Verify final state
    const auto &stats = allocator->GetDetailedStats();
    EXPECT_EQ(stats.currentUsage, 0);
    EXPECT_GT(successCount, 0);
}

TEST_F(HeapAllocatorTest, Defragmentation) {
    // Create fragmentation pattern
    std::vector<void *> ptrs;
    const int numAllocs = 100;

    // Allocate with varying sizes
    for (int i = 0; i < numAllocs; ++i) {
        ptrs.push_back(
            allocator->Allocate((i % 3 + 1) * 128));  // 128, 256, or 384 bytes
    }

    // Free every other allocation to create gaps
    for (int i = 0; i < numAllocs; i += 2) {
        allocator->Deallocate(ptrs[i]);
        ptrs[i] = nullptr;
    }

    // Measure fragmentation
    double fragRatioBefore = allocator->GetFragmentationRatio();

    // Trigger defragmentation
    allocator->Defragment();

    // Check if fragmentation improved
    double fragRatioAfter = allocator->GetFragmentationRatio();
    EXPECT_LT(fragRatioAfter, fragRatioBefore);

    // Cleanup remaining allocations
    for (void *ptr : ptrs) {
        if (ptr) allocator->Deallocate(ptr);
    }
}

TEST_F(HeapAllocatorTest, DetailedStats) {
    // Test detailed memory statistics
    void *ptr1 = allocator->AllocateTracked(
        100, 8, "Type1", AllocFlags::None, {"file1.cpp", 10, "Type1"});
    void *ptr2 = allocator->AllocateTracked(
        200, 16, "Type2", AllocFlags::None, {"file2.cpp", 20, "Type2"});

    const auto &stats = allocator->GetDetailedStats();

    // Check basic stats
    EXPECT_EQ(stats.totalAllocated, 300);
    EXPECT_EQ(stats.currentUsage, 300);
    EXPECT_EQ(stats.allocationCount, 2);

    // Check type-specific stats
    auto type1It = stats.allocationsByType.find("Type1");
    auto type2It = stats.allocationsByType.find("Type2");
    ASSERT_NE(type1It, stats.allocationsByType.end());
    ASSERT_NE(type2It, stats.allocationsByType.end());
    EXPECT_EQ(type1It->second, 100);
    EXPECT_EQ(type2It->second, 200);

    // Check file-specific stats
    auto file1It = stats.allocationsByFile.find("file1.cpp");
    auto file2It = stats.allocationsByFile.find("file2.cpp");
    ASSERT_NE(file1It, stats.allocationsByFile.end());
    ASSERT_NE(file2It, stats.allocationsByFile.end());
    EXPECT_EQ(file1It->second, 100);
    EXPECT_EQ(file2It->second, 200);

    // Check overhead stats
    EXPECT_GT(stats.headerOverhead, 0);
    EXPECT_GT(stats.guardOverhead, 0);
    EXPECT_GE(stats.wastedMemory, 0);

    allocator->Deallocate(ptr1);
    allocator->Deallocate(ptr2);
}

TEST_F(HeapAllocatorTest, ZeroMemory) {
    // Test zero memory flag
    const size_t size = 1024;
    void *ptr =
        allocator->Allocate(size, DEFAULT_ALIGNMENT, AllocFlags::ZeroMemory);
    ASSERT_NE(ptr, nullptr);

    // Verify memory is zeroed
    const char *charPtr = static_cast<const char *>(ptr);
    for (size_t i = 0; i < size; ++i) {
        EXPECT_EQ(charPtr[i], 0) << "Memory not zeroed at index " << i;
    }

    allocator->Deallocate(ptr);
}

TEST_F(HeapAllocatorTest, LeakDetection) {
    // Test memory leak detection
    void *ptr = allocator->AllocateTracked(
        100, 8, "LeakTest", AllocFlags::None, {"leak_test.cpp", 1, "LeakTest"});
    ASSERT_NE(ptr, nullptr);

    // Don't deallocate ptr to create a leak
    testing::internal::CaptureStderr();
    allocator.reset();  // This should trigger leak detection
    std::string output = testing::internal::GetCapturedStderr();

    EXPECT_TRUE(output.find("Memory leaks detected") != std::string::npos);
    EXPECT_TRUE(output.find("leak_test.cpp") != std::string::npos);
    EXPECT_TRUE(output.find("LeakTest") != std::string::npos);
}

TEST_F(HeapAllocatorTest, Reset) {
    // Test allocator reset
    std::vector<void *> ptrs;
    for (int i = 0; i < 10; ++i) {
        ptrs.push_back(allocator->Allocate(100));
    }

    const auto &statsBefore = allocator->GetDetailedStats();
    EXPECT_GT(statsBefore.currentUsage, 0);
    EXPECT_GT(statsBefore.allocationCount, 0);

    allocator->Reset();

    const auto &statsAfter = allocator->GetDetailedStats();
    EXPECT_EQ(statsAfter.currentUsage, 0);
    EXPECT_EQ(statsAfter.allocationCount, 0);
    EXPECT_EQ(statsAfter.totalAllocated, 0);
}
