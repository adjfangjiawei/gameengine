
#pragma once

#include <atomic>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "Core/Memory/MemoryTypes.h"
#include "Core/Memory/MovableMutex.h"
#include "Core/Memory/PoolAllocator.h"
#include "CoreTypes.h"

namespace Engine {
    namespace Memory {

        class MemoryPoolManager {
          public:
            struct Config {
                size_t minBlockSize{16};        // Minimum block size for pools
                size_t maxBlockSize{16384};     // Maximum block size for pools
                size_t blockSizeMultiplier{2};  // Multiplier between pool sizes
                size_t initialPoolSize{1024};   // Initial size of each pool
                size_t maxPoolSize{1024 * 1024};  // Maximum size of each pool
                bool enableThreadLocal{true};     // Enable thread-local pools
                size_t maxThreadPools{
                    32};  // Maximum number of thread-local pools
                bool enablePoolReuse{true};    // Enable pool reuse
                float growthFactor{1.5f};      // Pool growth factor
                float shrinkThreshold{0.25f};  // Pool shrink threshold
            };

            struct PoolStats {
                size_t blockSize{0};      // Size of blocks in this pool
                size_t totalBlocks{0};    // Total number of blocks
                size_t usedBlocks{0};     // Number of used blocks
                size_t freeBlocks{0};     // Number of free blocks
                size_t poolSize{0};       // Total size of pool memory
                size_t wastedMemory{0};   // Wasted memory due to fragmentation
                float utilization{0.0f};  // Pool utilization ratio
                uint64_t allocations{0};  // Total number of allocations
                uint64_t deallocations{0};  // Total number of deallocations
                double averageLatency{0};   // Average allocation latency
            };

            struct ThreadPoolInfo {
                uint32_t threadId{0};         // Thread ID
                size_t activeAllocations{0};  // Number of active allocations
                size_t totalAllocated{0};     // Total bytes allocated
                size_t peakUsage{0};          // Peak memory usage
                std::vector<PoolStats> poolStats;  // Stats for each pool
            };

            static MemoryPoolManager &Get();

            void Initialize(const Config &config);
            void Shutdown();

            // Allocation/deallocation
            void *Allocate(size_t size, size_t alignment = DEFAULT_ALIGNMENT);
            void Deallocate(void *ptr);
            void *AllocateInPool(size_t poolIndex,
                                 size_t alignment = DEFAULT_ALIGNMENT);
            void DeallocateFromPool(void *ptr, size_t poolIndex);

            // Pool management
            size_t CreatePool(size_t blockSize, size_t initialBlocks);
            void DestroyPool(size_t poolIndex);
            void GrowPool(size_t poolIndex);
            void ShrinkPool(size_t poolIndex);
            void OptimizePools();

            // Statistics and monitoring
            PoolStats GetPoolStats(size_t poolIndex) const;
            std::vector<ThreadPoolInfo> GetThreadPoolStats() const;
            size_t GetOptimalPoolIndex(size_t size) const;
            void DumpStats() const;

            // Pool class representing a memory pool with its allocator and
            // stats
            class Pool {
              public:
                PoolAllocator allocator;
                PoolStats stats;
                MovableMutex mutex;
                bool isThreadLocal{false};

                // Constructor with config
                explicit Pool(const PoolAllocator::Config &config)
                    : allocator(config), isThreadLocal(false) {}

                // Delete copy constructor and assignment operator
                Pool(const Pool &) = delete;
                Pool &operator=(const Pool &) = delete;

                // Define move constructor
                Pool(Pool &&other) noexcept
                    : allocator(std::move(other.allocator)),
                      stats(std::move(other.stats)),
                      mutex(std::move(other.mutex)),
                      isThreadLocal(other.isThreadLocal) {}

                // Define move assignment operator
                Pool &operator=(Pool &&other) noexcept {
                    if (this != &other) {
                        allocator = std::move(other.allocator);
                        stats = std::move(other.stats);
                        mutex = std::move(other.mutex);
                        isThreadLocal = other.isThreadLocal;
                    }
                    return *this;
                }
            };

          public:
            struct ThreadLocalPools {
                uint32_t threadId{0};
                std::vector<Pool> pools;
                std::chrono::steady_clock::time_point lastAccess;
            };

          private:
            MemoryPoolManager() = default;
            ~MemoryPoolManager() = default;
            MemoryPoolManager(const MemoryPoolManager &) = delete;
            MemoryPoolManager &operator=(const MemoryPoolManager &) = delete;

            // Performance tracking
            struct PerformanceMetrics {
                std::atomic<uint64_t> totalAllocations{0};
                std::atomic<uint64_t> totalDeallocations{0};
                std::atomic<uint64_t> totalAllocationTime{0};
                std::atomic<uint64_t> cacheMisses{0};
                std::atomic<uint64_t> poolGrowths{0};
                std::atomic<uint64_t> poolShrinks{0};
            } metrics;

            Config config;
            std::vector<Pool> globalPools;
            std::unordered_map<uint32_t, ThreadLocalPools> threadPools;
            mutable std::mutex globalMutex;

            // Thread-local storage
            static thread_local ThreadLocalPools *currentThreadPools;

            // Internal helpers
            Pool *FindOrCreateThreadLocalPool(size_t size);
            void UpdatePoolStats(Pool &pool);
            void CleanupUnusedPools();
            bool ShouldGrowPool(const Pool &pool) const;
            bool ShouldShrinkPool(const Pool &pool) const;
            size_t CalculateOptimalBlockSize(size_t requestedSize) const;
            void LogPoolOperation(const char *operation,
                                  size_t poolIndex,
                                  size_t size);

            // Thread-local storage management
            ThreadLocalPools &GetThreadLocalPools();
            void RegisterThreadPools(uint32_t threadId);
            void UnregisterThreadPools(uint32_t threadId);
        };

    }  // namespace Memory
}  // namespace Engine
