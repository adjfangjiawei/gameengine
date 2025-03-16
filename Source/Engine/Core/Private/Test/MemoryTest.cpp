
#include <chrono>
#include <thread>
#include <vector>

#include "Assert.h"
#include "Log/LogSystem.h"
#include "Memory/HeapAllocator.h"
#include "Memory/LinearAllocator.h"
#include "Memory/MemoryDebugger.h"
#include "Memory/MemoryPoolManager.h"
#include "Memory/MemoryProfiler.h"
#include "Memory/PoolAllocator.h"
#include "Memory/StackAllocator.h"

using namespace Engine::Memory;

// Test helper functions
void TestLinearAllocator() {
    LOG_INFO("Testing LinearAllocator...");

    LinearAllocator::Config config;
    config.totalSize = 1024;
    config.allowReset = true;

    LinearAllocator allocator(config);

    // Test basic allocation
    void *ptr1 = allocator.Allocate(128);
    ASSERT_MSG(ptr1 != nullptr,
               "Allocation should succeed",
               Engine::AssertType::Error);
    ASSERT_MSG(allocator.GetAllocationSize(ptr1) == 128,
               "Allocation size should match",
               Engine::AssertType::Error);

    void *ptr2 = allocator.Allocate(256);
    ASSERT_MSG(ptr2 != nullptr,
               "Second allocation should succeed",
               Engine::AssertType::Error);
    ASSERT_MSG(allocator.GetAllocationSize(ptr2) == 256,
               "Second allocation size should match",
               Engine::AssertType::Error);

    // Test alignment
    void *aligned = allocator.Allocate(64, 16);
    ASSERT_MSG(aligned != nullptr,
               "Aligned allocation should succeed",
               Engine::AssertType::Error);
    ASSERT_MSG(reinterpret_cast<uintptr_t>(aligned) % 16 == 0,
               "Alignment should be correct",
               Engine::AssertType::Error);

    // Test reset
    allocator.Reset();
    void *ptr3 = allocator.Allocate(128);
    ASSERT_MSG(ptr3 != nullptr,
               "Post-reset allocation should succeed",
               Engine::AssertType::Error);
    ASSERT_MSG(ptr3 == ptr1,
               "Should reuse the same memory after reset",
               Engine::AssertType::Error);

    LOG_INFO("LinearAllocator tests passed");
}

void TestStackAllocator() {
    LOG_INFO("Testing StackAllocator...");

    StackAllocator::Config config;
    config.totalSize = 1024;
    config.stackDepth = 16;

    StackAllocator allocator(config);

    // Test LIFO behavior
    void *ptr1 = allocator.Allocate(128);
    ASSERT_MSG(ptr1 != nullptr,
               "First LIFO allocation should succeed",
               Engine::AssertType::Error);

    void *ptr2 = allocator.Allocate(256);
    ASSERT_MSG(ptr2 != nullptr,
               "Second LIFO allocation should succeed",
               Engine::AssertType::Error);

    // Deallocate in reverse order
    allocator.Deallocate(ptr2);
    allocator.Deallocate(ptr1);

    // Test marker functionality
    size_t marker = allocator.GetMarker();
    void *ptr3 = allocator.Allocate(128);
    void *ptr4 = allocator.Allocate(128);
    ASSERT_MSG(ptr3 != nullptr && ptr4 != nullptr,
               "Marker test allocations should succeed",
               Engine::AssertType::Error);

    allocator.FreeToMarker(marker);
    void *ptr5 = allocator.Allocate(128);
    ASSERT_MSG(ptr5 == ptr3,
               "Should reuse the same memory after freeing to marker",
               Engine::AssertType::Error);

    LOG_INFO("StackAllocator tests passed");
}

void TestHeapAllocator() {
    LOG_INFO("Testing HeapAllocator...");

    HeapAllocator::Config config;
    config.trackAllocations = true;
    config.enableDebugInfo = true;

    HeapAllocator allocator(config);

    // Test allocation with tracking
    void *ptr1 = allocator.AllocateTracked(128, 8, "TestBlock");
    ASSERT(ptr1 != nullptr, "Allocation should succeed");
    ASSERT(allocator.GetAllocationSize(ptr1) == 128,
           "Allocation size should match");

    // Test debug info
    allocator.DumpAllocationInfo();
    allocator.ValidateHeap();

    // Test deallocation
    allocator.Deallocate(ptr1);
    allocator.CheckForLeaks();

    LOG_INFO("HeapAllocator tests passed");
}

void TestPoolAllocator() {
    LOG_INFO("Testing PoolAllocator...");

    PoolAllocator::Config config;
    config.blockSize = 64;
    config.initialChunks = 16;
    config.expansionChunks = 8;

    PoolAllocator allocator(config);

    // Test multiple allocations
    std::vector<void *> pointers;
    for (int i = 0; i < 20; ++i) {
        void *ptr = allocator.Allocate(64);
        ASSERT_MSG(ptr != nullptr,
                   "Pool allocation should succeed",
                   Engine::AssertType::Error);
        pointers.push_back(ptr);
    }

    // Test pool expansion
    auto stats = allocator.GetStats();
    ASSERT_MSG(stats.totalBlocks >= 24,
               "Pool should have expanded",
               Engine::AssertType::Error);

    // Test deallocations
    for (void *ptr : pointers) {
        allocator.Deallocate(ptr);
    }

    // Test pool shrinking
    allocator.Shrink();
    stats = allocator.GetStats();
    ASSERT_MSG(stats.totalBlocks == config.initialChunks,
               "Pool should shrink to initial size",
               Engine::AssertType::Error);

    LOG_INFO("PoolAllocator tests passed");
}

void TestMemoryPoolManager() {
    LOG_INFO("Testing MemoryPoolManager...");

    MemoryPoolManager::Config config;
    config.minBlockSize = 16;
    config.maxBlockSize = 1024;
    config.enableThreadLocal = true;

    auto &manager = MemoryPoolManager::Get();
    manager.Initialize(config);

    // Test allocation from different pools
    void *small = manager.Allocate(32);
    void *medium = manager.Allocate(256);
    void *large = manager.Allocate(512);

    ASSERT_MSG(small != nullptr && medium != nullptr && large != nullptr,
               "All pool allocations should succeed",
               Engine::AssertType::Error);

    // Test thread-local pools
    std::thread worker([&manager]() {
        void *ptr = manager.Allocate(64);
        ASSERT_MSG(ptr != nullptr,
                   "Thread-local allocation should succeed",
                   Engine::AssertType::Error);
        manager.Deallocate(ptr);
    });
    worker.join();

    // Test deallocation
    manager.Deallocate(small);
    manager.Deallocate(medium);
    manager.Deallocate(large);

    // Test pool stats
    manager.DumpStats();

    LOG_INFO("MemoryPoolManager tests passed");
}

void TestMemoryDebugger() {
    LOG_INFO("Testing MemoryDebugger...");
    LOG_INFO("Testing MemoryDebugger...");

    MemoryDebugger::Config config;
    config.enableBoundaryChecking = true;
    config.enableAccessTracking = true;

    auto &debugger = MemoryDebugger::Get();
    debugger.Initialize(config);

    // Test allocation tracking
    void *ptr = std::malloc(128);
    debugger.OnAllocation(ptr, 128, 8, "TestBlock", MEMORY_LOCATION);

    // Test access tracking
    debugger.OnAccess(ptr, 64, true, MEMORY_LOCATION);
    debugger.OnAccess(ptr, 32, false, MEMORY_LOCATION);

    // Test boundary checking
    debugger.ValidateAllAllocations();

    // Test deallocation
    debugger.OnDeallocation(ptr, MEMORY_LOCATION);
    std::free(ptr);

    // Generate report
    // debugger.GenerateReport("memory_debug_report.txt");

    LOG_INFO("MemoryDebugger tests passed");
}

void TestMemoryProfiler() {
    LOG_INFO("Testing MemoryProfiler...");

    MemoryProfiler::Config config;
    config.enableDetailedTracking = true;
    config.enableHotspotDetection = true;

    auto &profiler = MemoryProfiler::Get();
    profiler.Initialize(config);
    profiler.StartProfiling();

    // Generate some memory activity
    for (int i = 0; i < 100; ++i) {
        void *ptr = std::malloc(64);
        profiler.OnAllocation(ptr, 64, "TestBlock");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        profiler.OnDeallocation(ptr);
        std::free(ptr);
    }

    // Test performance metrics
    auto metrics = profiler.GetPerformanceMetrics();
    ASSERT_MSG(metrics.totalAllocationTime > 0,
               "Should have recorded allocations",
               Engine::AssertType::Error);
    ASSERT_MSG(metrics.totalAllocationTime > 0,
               "Should have recorded deallocations",
               Engine::AssertType::Error);

    // Test hotspot detection
    auto hotspots = profiler.GetHotspots();
    profiler.DumpStatistics();

    profiler.StopProfiling();

    LOG_INFO("MemoryProfiler tests passed");
}

int main() {
    LOG_INFO("Starting memory management tests...");

    try {
        TestLinearAllocator();
        TestStackAllocator();
        TestHeapAllocator();
        TestPoolAllocator();
        TestMemoryPoolManager();
        TestMemoryDebugger();
        TestMemoryProfiler();

        LOG_INFO("All memory management tests passed!");
        return 0;
    } catch (const std::exception &e) {
        LOG_ERROR("Test failed with exception: {}", e.what());
        return 1;
    }
}
