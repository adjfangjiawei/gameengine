
#pragma once

#include <chrono>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "Core/Memory/MemoryTypes.h"
#include "CoreTypes.h"

namespace Engine {
    namespace Memory {

        class MemoryAnalyzer {
          public:
            struct Config {
                bool trackAllocationHistory{true};  // Track allocation history
                bool trackAccessPatterns{true};  // Track memory access patterns
                bool trackPerformance{true};     // Track performance metrics
                size_t maxHistoryEntries{
                    10000};  // Maximum number of history entries
                size_t sampleInterval{
                    1000};  // Sampling interval in milliseconds
            };

            struct AllocationMetrics {
                size_t count;      // Number of allocations
                size_t totalSize;  // Total size allocated
                size_t minSize;    // Minimum allocation size
                size_t maxSize;    // Maximum allocation size
                double avgSize;    // Average allocation size
                double allocRate;  // Allocations per second
                double freeRate;   // Deallocations per second
                uint64 totalTime;  // Total time spent in allocations
                uint64 maxTime;    // Maximum allocation time
                double avgTime;    // Average allocation time
            };

            struct FragmentationMetrics {
                size_t totalFragments;    // Total number of memory fragments
                size_t largestFragment;   // Size of largest fragment
                size_t smallestFragment;  // Size of smallest fragment
                double fragmentation;     // Fragmentation ratio (0.0 - 1.0)
                size_t wastedSpace;  // Total wasted space due to fragmentation
                double utilization;  // Memory utilization ratio
            };

            struct AccessMetrics {
                size_t readCount;         // Number of read operations
                size_t writeCount;        // Number of write operations
                size_t sequentialAccess;  // Number of sequential accesses
                size_t randomAccess;      // Number of random accesses
                double readWriteRatio;    // Ratio of reads to writes
                std::vector<size_t>
                    hotSpots;  // Memory addresses with high access frequency
            };

            struct TimeMetrics {
                uint64 totalAllocationTime;  // Total time spent in allocations
                uint64
                    totalDeallocationTime;  // Total time spent in deallocations
                uint64 maxAllocationTime;   // Maximum allocation time
                uint64 maxDeallocationTime;  // Maximum deallocation time
                double avgAllocationTime;    // Average allocation time
                double avgDeallocationTime;  // Average deallocation time
                size_t slowAllocations;      // Number of slow allocations
                size_t slowDeallocations;    // Number of slow deallocations
            };

            struct MemorySnapshot {
                size_t timestamp;
                size_t totalAllocated;
                size_t totalFreed;
                size_t currentUsage;
                size_t peakUsage;
                size_t allocations;
                size_t deallocations;
                FragmentationMetrics fragmentation;
            };

            static MemoryAnalyzer &Get();

            void Initialize(const Config &config);
            void Shutdown();

            // Event tracking
            void OnAllocation(void *ptr,
                              size_t size,
                              const char *typeName,
                              const MemoryLocation &location);
            void OnDeallocation(void *ptr, const MemoryLocation &location);
            void OnAccess(void *ptr,
                          size_t size,
                          bool isWrite,
                          const MemoryLocation &location);

            // Analysis functions
            AllocationMetrics GetAllocationMetrics() const;
            FragmentationMetrics GetFragmentationMetrics() const;
            AccessMetrics GetAccessMetrics() const;
            TimeMetrics GetTimeMetrics() const;
            std::vector<MemorySnapshot> GetHistory() const;

            // Report generation
            void GenerateReport(const char *filename) const;
            void DumpStatistics() const;
            void ResetStatistics();

            // Analysis helpers
            void TakeSnapshot();
            void AnalyzeFragmentation();
            void AnalyzeHotSpots();
            void AnalyzePerformance();

            // Memory usage tracking
            size_t GetCurrentUsage() const;
            size_t GetPeakUsage() const;

          private:
            MemoryAnalyzer() = default;
            ~MemoryAnalyzer() = default;
            MemoryAnalyzer(const MemoryAnalyzer &) = delete;
            MemoryAnalyzer &operator=(const MemoryAnalyzer &) = delete;

            struct AllocationInfo {
                size_t size;
                const char *typeName;
                MemoryLocation location;
                uint64 timestamp;
                uint64 allocationTime;
            };

            struct AccessInfo {
                uint64 timestamp;
                bool isWrite;
                size_t size;
                MemoryLocation location;
            };

            struct SizeDistribution {
                std::vector<size_t> sizes;
                std::vector<size_t> counts;
            };

            void UpdateMetrics();
            void UpdateFragmentationMetrics();
            void UpdateAccessMetrics();
            void UpdateTimeMetrics();
            SizeDistribution CalculateSizeDistribution() const;
            void DetectMemoryLeaks() const;
            void AnalyzeAccessPatterns() const;

            Config config;
            std::unordered_map<void *, AllocationInfo> allocations;
            std::unordered_map<void *, std::vector<AccessInfo>> accessHistory;
            std::vector<MemorySnapshot> history;

            AllocationMetrics allocationMetrics;
            FragmentationMetrics fragmentationMetrics;
            AccessMetrics accessMetrics;
            TimeMetrics timeMetrics;

            std::chrono::steady_clock::time_point lastUpdate;
            mutable std::mutex mutex;
        };

// Helper macros for memory analysis
#if BUILD_DEBUG
#define TRACK_ALLOCATION(ptr, size, type)               \
    Engine::Memory::MemoryAnalyzer::Get().OnAllocation( \
        ptr, size, type, MEMORY_LOCATION)
#define TRACK_DEALLOCATION(ptr) \
    Engine::Memory::MemoryAnalyzer::Get().OnDeallocation(ptr, MEMORY_LOCATION)
#define ANALYZE_MEMORY() \
    Engine::Memory::MemoryAnalyzer::Get().AnalyzeFragmentation()
#else
#define TRACK_ALLOCATION(ptr, size, type) ((void)0)
#define TRACK_DEALLOCATION(ptr) ((void)0)
#define ANALYZE_MEMORY() ((void)0)
#endif

    }  // namespace Memory
}  // namespace Engine
