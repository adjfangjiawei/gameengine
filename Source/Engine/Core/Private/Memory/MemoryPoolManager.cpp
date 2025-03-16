
#include "Memory/MemoryPoolManager.h"

#include <algorithm>
#include <numeric>

#include "Assert.h"
#include "Log/LogSystem.h"

namespace Engine {
    namespace Memory {

        // Using declarations to simplify code
        using ThreadLocalPools = MemoryPoolManager::ThreadLocalPools;
        using Pool = MemoryPoolManager::Pool;
        using PoolStats = MemoryPoolManager::PoolStats;
        using ThreadPoolInfo = MemoryPoolManager::ThreadPoolInfo;

        // Initialize thread-local storage
        thread_local MemoryPoolManager::ThreadLocalPools
            *MemoryPoolManager::currentThreadPools = nullptr;

        MemoryPoolManager &MemoryPoolManager::Get() {
            static MemoryPoolManager instance;
            return instance;
        }

        void MemoryPoolManager::Initialize(const Config &config) {
            std::lock_guard<std::mutex> lock(globalMutex);
            this->config = config;

            // Create initial global pools
            size_t blockSize = config.minBlockSize;
            while (blockSize <= config.maxBlockSize) {
                CreatePool(blockSize, config.initialPoolSize / blockSize);
                blockSize *= config.blockSizeMultiplier;
            }
        }

        void MemoryPoolManager::Shutdown() {
            std::lock_guard<std::mutex> lock(globalMutex);

            // Cleanup global pools
            globalPools.clear();

            // Cleanup thread-local pools
            threadPools.clear();
            currentThreadPools = nullptr;

            DumpStats();
        }

        void *MemoryPoolManager::Allocate(size_t size, size_t alignment) {
            auto start = std::chrono::steady_clock::now();

            // Try thread-local pools first
            if (config.enableThreadLocal) {
                if (Pool *pool = FindOrCreateThreadLocalPool(size)) {
                    void *ptr = pool->allocator.Allocate(alignment);
                    if (ptr) {
                        auto end = std::chrono::steady_clock::now();
                        metrics.totalAllocationTime +=
                            std::chrono::duration_cast<
                                std::chrono::nanoseconds>(end - start)
                                .count();
                        metrics.totalAllocations++;
                        return ptr;
                    }
                }
            }

            // Fall back to global pools
            size_t poolIndex = GetOptimalPoolIndex(size);
            if (poolIndex < globalPools.size()) {
                // Use the lock method directly on MovableMutex
                globalPools[poolIndex].mutex.lock();
                void *ptr =
                    globalPools[poolIndex].allocator.Allocate(alignment);
                globalPools[poolIndex].mutex.unlock();

                if (ptr) {
                    auto end = std::chrono::steady_clock::now();
                    metrics.totalAllocationTime +=
                        std::chrono::duration_cast<std::chrono::nanoseconds>(
                            end - start)
                            .count();
                    metrics.totalAllocations++;
                    return ptr;
                }

                // Try growing the pool if allocation failed
                if (ShouldGrowPool(globalPools[poolIndex])) {
                    GrowPool(poolIndex);

                    globalPools[poolIndex].mutex.lock();
                    ptr = globalPools[poolIndex].allocator.Allocate(alignment);
                    globalPools[poolIndex].mutex.unlock();

                    if (ptr) {
                        auto end = std::chrono::steady_clock::now();
                        metrics.totalAllocationTime +=
                            std::chrono::duration_cast<
                                std::chrono::nanoseconds>(end - start)
                                .count();
                        metrics.totalAllocations++;
                        return ptr;
                    }
                }
            }

            metrics.cacheMisses++;
            LOG_ERROR("Failed to allocate {} bytes", size);
            return nullptr;
        }

        void MemoryPoolManager::Deallocate(void *ptr) {
            if (!ptr) return;

            [[maybe_unused]] auto start = std::chrono::steady_clock::now();

            // Try thread-local pools first
            if (config.enableThreadLocal && currentThreadPools) {
                for (auto &pool : currentThreadPools->pools) {
                    if (pool.allocator.OwnsAllocation(ptr)) {
                        pool.allocator.Deallocate(ptr);
                        metrics.totalDeallocations++;
                        return;
                    }
                }
            }

            // Try global pools
            for (size_t i = 0; i < globalPools.size(); ++i) {
                if (globalPools[i].allocator.OwnsAllocation(ptr)) {
                    // Use a std::lock_guard with the internal mutex
                    globalPools[i].mutex.lock();
                    globalPools[i].allocator.Deallocate(ptr);
                    metrics.totalDeallocations++;

                    // Check if pool should be shrunk
                    if (ShouldShrinkPool(globalPools[i])) {
                        ShrinkPool(i);
                    }
                    globalPools[i].mutex.unlock();
                    return;
                }
            }

            LOG_WARNING("Attempting to deallocate unknown pointer");
        }

        void *MemoryPoolManager::AllocateInPool(size_t poolIndex,
                                                size_t alignment) {
            if (poolIndex >= globalPools.size()) {
                return nullptr;
            }

            globalPools[poolIndex].mutex.lock();
            void *ptr = globalPools[poolIndex].allocator.Allocate(alignment);

            if (ptr) {
                metrics.totalAllocations++;
                globalPools[poolIndex].mutex.unlock();
                return ptr;
            }

            if (ShouldGrowPool(globalPools[poolIndex])) {
                // Need to unlock before calling GrowPool which will lock again
                globalPools[poolIndex].mutex.unlock();
                GrowPool(poolIndex);

                // Lock again to allocate
                globalPools[poolIndex].mutex.lock();
                ptr = globalPools[poolIndex].allocator.Allocate(alignment);
                globalPools[poolIndex].mutex.unlock();

                if (ptr) {
                    metrics.totalAllocations++;
                    return ptr;
                }
            } else {
                globalPools[poolIndex].mutex.unlock();
            }

            return nullptr;
        }

        void MemoryPoolManager::DeallocateFromPool(void *ptr,
                                                   size_t poolIndex) {
            if (!ptr || poolIndex >= globalPools.size()) {
                return;
            }

            globalPools[poolIndex].mutex.lock();
            if (globalPools[poolIndex].allocator.OwnsAllocation(ptr)) {
                globalPools[poolIndex].allocator.Deallocate(ptr);
                metrics.totalDeallocations++;

                bool shouldShrink = ShouldShrinkPool(globalPools[poolIndex]);
                globalPools[poolIndex].mutex.unlock();

                if (shouldShrink) {
                    ShrinkPool(poolIndex);
                }
            } else {
                globalPools[poolIndex].mutex.unlock();
                LOG_WARNING("Pointer does not belong to specified pool");
            }
        }

        size_t MemoryPoolManager::CreatePool(size_t blockSize,
                                             size_t initialBlocks) {
            PoolAllocator::Config config;
            config.blockSize = blockSize;
            config.initialChunks = initialBlocks;
            config.expansionChunks = initialBlocks / 2;
            config.maxChunks = this->config.maxPoolSize / blockSize;
            config.alignment = DEFAULT_ALIGNMENT;
            config.allowExpansion = true;

            Pool pool(std::move(config));
            globalPools.push_back(std::move(pool));
            return globalPools.size() - 1;
        }

        void MemoryPoolManager::DestroyPool(size_t poolIndex) {
            if (poolIndex >= globalPools.size()) {
                return;
            }

            // Need to lock the global mutex since we're modifying the
            // globalPools vector
            std::lock_guard<std::mutex> lock(globalMutex);
            globalPools.erase(globalPools.begin() + poolIndex);
        }

        void MemoryPoolManager::GrowPool(size_t poolIndex) {
            if (poolIndex >= globalPools.size()) {
                return;
            }

            globalPools[poolIndex].mutex.lock();
            Pool &pool = globalPools[poolIndex];

            if (pool.allocator.Expand(config.initialPoolSize /
                                      pool.stats.blockSize)) {
                metrics.poolGrowths++;
                UpdatePoolStats(pool);
                LogPoolOperation("Grow",
                                 poolIndex,
                                 pool.stats.totalBlocks * pool.stats.blockSize);
            }
            globalPools[poolIndex].mutex.unlock();
        }

        void MemoryPoolManager::ShrinkPool(size_t poolIndex) {
            if (poolIndex >= globalPools.size()) {
                return;
            }

            globalPools[poolIndex].mutex.lock();
            Pool &pool = globalPools[poolIndex];

            pool.allocator.Shrink();
            metrics.poolShrinks++;
            UpdatePoolStats(pool);
            LogPoolOperation("Shrink",
                             poolIndex,
                             pool.stats.totalBlocks * pool.stats.blockSize);

            globalPools[poolIndex].mutex.unlock();
        }

        void MemoryPoolManager::OptimizePools() {
            std::lock_guard<std::mutex> lock(globalMutex);

            // Clean up unused thread-local pools
            CleanupUnusedPools();

            // Optimize global pools
            for (size_t i = 0; i < globalPools.size(); ++i) {
                if (ShouldShrinkPool(globalPools[i])) {
                    ShrinkPool(i);
                }
            }
        }

        MemoryPoolManager::PoolStats MemoryPoolManager::GetPoolStats(
            size_t poolIndex) const {
            if (poolIndex >= globalPools.size()) {
                return MemoryPoolManager::PoolStats();
            }

            // Use direct locking for const methods
            globalPools[poolIndex].mutex.lock();
            auto stats = globalPools[poolIndex].stats;
            globalPools[poolIndex].mutex.unlock();
            return stats;
        }

        std::vector<MemoryPoolManager::ThreadPoolInfo>
        MemoryPoolManager::GetThreadPoolStats() const {
            // Use direct locking for const method
            globalMutex.lock();
            std::vector<MemoryPoolManager::ThreadPoolInfo> result;

            for (const auto &pair : threadPools) {
                MemoryPoolManager::ThreadPoolInfo info;
                info.threadId = pair.first;

                for (const auto &pool : pair.second.pools) {
                    info.activeAllocations += pool.stats.usedBlocks;
                    info.totalAllocated +=
                        pool.stats.totalBlocks * pool.stats.blockSize;
                    info.peakUsage =
                        std::max(info.peakUsage,
                                 pool.stats.totalBlocks * pool.stats.blockSize);
                    info.poolStats.push_back(pool.stats);
                }

                result.push_back(info);
            }

            globalMutex.unlock();
            return result;
        }

        size_t MemoryPoolManager::GetOptimalPoolIndex(size_t size) const {
            size_t blockSize = config.minBlockSize;
            size_t index = 0;

            while (index < globalPools.size()) {
                if (size <= blockSize) {
                    return index;
                }
                blockSize *= config.blockSizeMultiplier;
                index++;
            }

            return SIZE_MAX;
        }

        void MemoryPoolManager::DumpStats() const {
            // Use direct locking for const method
            globalMutex.lock();

            LOG_INFO("Memory Pool Manager Statistics:");
            LOG_INFO("=============================");

            // Global pools
            LOG_INFO("Global Pools:");
            for (size_t i = 0; i < globalPools.size(); ++i) {
                const auto &stats = globalPools[i].stats;
                LOG_INFO(
                    "Pool {}: Block Size: {}, Used: {}/{}, Utilization: "
                    "{:.2f}%",
                    i,
                    stats.blockSize,
                    stats.usedBlocks,
                    stats.totalBlocks,
                    stats.utilization * 100.0f);
            }

            // Thread pools
            LOG_INFO("\nThread-Local Pools:");
            for (const auto &pair : threadPools) {
                LOG_INFO("Thread {}: {} pools, {} total bytes",
                         pair.first,
                         pair.second.pools.size(),
                         std::accumulate(pair.second.pools.begin(),
                                         pair.second.pools.end(),
                                         size_t{0},
                                         [](size_t sum, const Pool &pool) {
                                             return sum +
                                                    pool.stats.totalBlocks *
                                                        pool.stats.blockSize;
                                         }));
            }

            // Performance metrics
            LOG_INFO("\nPerformance Metrics:");
            LOG_INFO("Total Allocations: {}", metrics.totalAllocations.load());
            LOG_INFO("Total Deallocations: {}",
                     metrics.totalDeallocations.load());
            LOG_INFO(
                "Average Allocation Time: {:.2f} ns",
                metrics.totalAllocations.load() > 0
                    ? static_cast<double>(metrics.totalAllocationTime.load()) /
                          metrics.totalAllocations.load()
                    : 0.0);
            LOG_INFO("Cache Misses: {}", metrics.cacheMisses.load());
            LOG_INFO("Pool Growths: {}", metrics.poolGrowths.load());
            LOG_INFO("Pool Shrinks: {}", metrics.poolShrinks.load());

            globalMutex.unlock();
        }

        MemoryPoolManager::Pool *MemoryPoolManager::FindOrCreateThreadLocalPool(
            size_t size) {
            if (!currentThreadPools) {
                uint32_t threadId = static_cast<uint32_t>(
                    std::hash<std::thread::id>{}(std::this_thread::get_id()));
                RegisterThreadPools(threadId);
            }

            if (!currentThreadPools) {
                return nullptr;
            }

            // Find existing pool that can handle this size
            for (auto &pool : currentThreadPools->pools) {
                if (size <= pool.stats.blockSize) {
                    return &pool;
                }
            }

            // Create new pool if possible
            if (currentThreadPools->pools.size() < config.maxThreadPools) {
                size_t blockSize = CalculateOptimalBlockSize(size);
                PoolAllocator::Config allocConfig;
                allocConfig.blockSize = blockSize;
                allocConfig.initialChunks = config.initialPoolSize / blockSize;
                allocConfig.expansionChunks = allocConfig.initialChunks / 2;
                allocConfig.maxChunks = config.maxPoolSize / blockSize;
                allocConfig.alignment = DEFAULT_ALIGNMENT;
                allocConfig.allowExpansion = true;

                currentThreadPools->pools.emplace_back(allocConfig);
                return &currentThreadPools->pools.back();
            }

            return nullptr;
        }

        void MemoryPoolManager::UpdatePoolStats(Pool &pool) {
            auto stats = pool.allocator.GetStats();
            pool.stats.blockSize = stats.blockSize;
            pool.stats.totalBlocks = stats.totalBlocks;
            pool.stats.usedBlocks = stats.usedBlocks;
            pool.stats.freeBlocks = stats.totalBlocks - stats.usedBlocks;
            pool.stats.poolSize = stats.totalBlocks * stats.blockSize;
            pool.stats.wastedMemory = stats.wastedMemory;
            pool.stats.utilization =
                stats.usedBlocks > 0
                    ? static_cast<float>(stats.usedBlocks) / stats.totalBlocks
                    : 0.0f;
        }

        void MemoryPoolManager::CleanupUnusedPools() {
            auto now = std::chrono::steady_clock::now();
            auto it = threadPools.begin();

            while (it != threadPools.end()) {
                if (std::chrono::duration_cast<std::chrono::seconds>(
                        now - it->second.lastAccess)
                        .count() > 60) {
                    it = threadPools.erase(it);
                } else {
                    ++it;
                }
            }
        }

        bool MemoryPoolManager::ShouldGrowPool(const Pool &pool) const {
            return pool.stats.freeBlocks < pool.stats.totalBlocks * 0.1f;
        }

        bool MemoryPoolManager::ShouldShrinkPool(const Pool &pool) const {
            return pool.stats.utilization < config.shrinkThreshold &&
                   pool.stats.totalBlocks >
                       config.initialPoolSize / pool.stats.blockSize;
        }

        size_t MemoryPoolManager::CalculateOptimalBlockSize(
            size_t requestedSize) const {
            size_t blockSize = config.minBlockSize;
            while (blockSize < requestedSize &&
                   blockSize < config.maxBlockSize) {
                blockSize *= config.blockSizeMultiplier;
            }
            return blockSize;
        }

        void MemoryPoolManager::LogPoolOperation(const char *operation,
                                                 size_t poolIndex,
                                                 size_t size) {
            LOG_DEBUG(
                "Pool {} {}: new size = {} bytes", poolIndex, operation, size);
        }

        MemoryPoolManager::ThreadLocalPools &
        MemoryPoolManager::GetThreadLocalPools() {
            if (!currentThreadPools) {
                uint32_t threadId = static_cast<uint32_t>(
                    std::hash<std::thread::id>{}(std::this_thread::get_id()));
                RegisterThreadPools(threadId);
            }
            return *currentThreadPools;
        }

        void MemoryPoolManager::RegisterThreadPools(uint32_t threadId) {
            // Use direct locking to avoid MovableMutex issue
            globalMutex.lock();
            auto &pools = threadPools[threadId];
            pools.threadId = threadId;
            pools.lastAccess = std::chrono::steady_clock::now();
            currentThreadPools = &pools;
            globalMutex.unlock();
        }

        void MemoryPoolManager::UnregisterThreadPools(uint32_t threadId) {
            // Use direct locking to avoid MovableMutex issue
            globalMutex.lock();
            threadPools.erase(threadId);
            if (currentThreadPools &&
                currentThreadPools->threadId == threadId) {
                currentThreadPools = nullptr;
            }
            globalMutex.unlock();
        }

    }  // namespace Memory
}  // namespace Engine
