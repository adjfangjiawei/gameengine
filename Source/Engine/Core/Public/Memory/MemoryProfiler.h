
#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "Core/Memory/MemoryTypes.h"
#include "CoreTypes.h"

namespace Engine {
    namespace Memory {

        class MemoryProfiler {
          public:
            struct Config {
                bool enableDetailedTracking{
                    true};  // Enable detailed performance tracking
                bool enableHotspotDetection{
                    true};  // Enable memory hotspot detection
                bool enableCallstackTracking{
                    true};  // Enable callstack tracking
                size_t sampleInterval{
                    100};                  // Sampling interval in milliseconds
                size_t maxSamples{10000};  // Maximum number of samples to keep
                size_t hotspotThreshold{
                    1000};  // Threshold for hotspot detection (accesses)
                bool enableThreadTracking{
                    true};  // Enable per-thread statistics
            };

            struct AllocationProfile {
                size_t size;            // Size of allocation
                uint64 timestamp;       // Time of allocation
                uint64 duration;        // Duration of allocation operation
                uint32 threadId;        // Thread that made the allocation
                void *callstack[32];    // Callstack at allocation point
                size_t callstackDepth;  // Depth of captured callstack
                const char *tag;        // Optional allocation tag
            };

            struct ThreadStats {
                size_t totalAllocations;     // Total number of allocations
                size_t totalDeallocations;   // Total number of deallocations
                size_t currentBytes;         // Current bytes allocated
                size_t peakBytes;            // Peak bytes allocated
                uint64 totalAllocationTime;  // Total time spent in allocations
                uint64
                    totalDeallocationTime;  // Total time spent in deallocations
                double averageAllocationTime;    // Average allocation time
                double averageDeallocationTime;  // Average deallocation time
            };

            struct PerformanceMetrics {
                // Timing metrics
                uint64 minAllocationTime;      // Minimum allocation time
                uint64 maxAllocationTime;      // Maximum allocation time
                uint64 totalAllocationTime;    // Total allocation time
                double averageAllocationTime;  // Average allocation time
                double
                    allocationLatencyP95;  // 95th percentile allocation latency
                double
                    allocationLatencyP99;  // 99th percentile allocation latency

                // Throughput metrics
                double allocationsPerSecond;    // Allocation rate
                double deallocationsPerSecond;  // Deallocation rate
                double bytesPerSecond;          // Memory throughput

                // Cache metrics
                double cacheHitRate;        // Cache hit rate
                double cacheMissRate;       // Cache miss rate
                uint64 cacheLineTransfers;  // Number of cache line transfers

                // System metrics
                uint64 pageFaults;          // Number of page faults
                uint64 systemCalls;         // Number of system calls
                double lockContentionRate;  // Lock contention rate
                PerformanceMetrics();
            };

            struct HotspotInfo {
                void *address;       // Memory address
                size_t accessCount;  // Number of accesses
                size_t totalBytes;   // Total bytes accessed
                bool isWrite;        // Whether it's a write hotspot
                std::vector<uint32>
                    threadIds;  // Threads accessing this location
            };

            static MemoryProfiler &Get();

            void Initialize(const Config &config);
            void Shutdown();

            // Profiling control
            void StartProfiling();
            void StopProfiling();
            void PauseProfiling();
            void ResumeProfiling();
            bool IsProfiling() const;

            // Event tracking
            void OnAllocation(void *ptr,
                              size_t size,
                              const char *tag = nullptr);
            void OnDeallocation(void *ptr);
            void OnMemoryAccess(void *ptr, size_t size, bool isWrite);

            // Analysis functions
            PerformanceMetrics GetPerformanceMetrics() const;
            std::vector<HotspotInfo> GetHotspots() const;
            ThreadStats GetThreadStats(uint32 threadId) const;
            std::vector<AllocationProfile> GetAllocationHistory() const;

            // Report generation
            void GenerateReport(const char *filename) const;
            void DumpStatistics() const;
            void ResetStatistics();

            // Real-time monitoring
            void EnableRealTimeMonitoring(bool enable);
            void SetPerformanceCallback(
                std::function<void(const PerformanceMetrics &)> callback);

          private:
            MemoryProfiler() = default;
            ~MemoryProfiler() = default;
            MemoryProfiler(const MemoryProfiler &) = delete;
            MemoryProfiler &operator=(const MemoryProfiler &) = delete;

            struct Sample {
                uint64 timestamp;
                size_t allocatedBytes;
                size_t deallocatedBytes;
                uint64 allocationTime;
                uint64 deallocationTime;
                uint32 threadId;
            };

            struct ThreadContext {
                ThreadStats stats;
                std::vector<Sample> samples;
                std::chrono::steady_clock::time_point lastUpdate;
            };

            // Internal helpers
            void UpdateMetrics();
            void ProcessSamples();
            void DetectHotspots();
            void CalculateLatencyPercentiles();
            void CaptureCallstack(void **callstack, size_t &depth);
            void UpdateThreadStats(uint32 threadId, const Sample &sample);
            void LogPerformanceEvent(const char *event, uint64 duration);
            double CalculateLockContention() const;

            // Monitoring
            void StartMonitoringThread();
            void StopMonitoringThread();
            void MonitoringThreadFunction();

            Config config;
            std::unordered_map<void *, AllocationProfile> allocations;
            std::unordered_map<uint32, ThreadContext> threadContexts;
            std::vector<Sample> globalSamples;
            std::vector<HotspotInfo> hotspots;
            PerformanceMetrics currentMetrics;

            // Performance tracking
            std::atomic<size_t> totalAllocations{0};
            std::atomic<size_t> totalDeallocations{0};
            std::atomic<uint64> totalAllocationTime{0};
            std::atomic<uint64> totalDeallocationTime{0};
            std::atomic<size_t> pageFaultCount{0};
            std::atomic<size_t> systemCallCount{0};
            std::atomic<size_t> lockContentionCount{0};

            // Monitoring state
            bool isRunning{false};
            bool isPaused{false};
            bool shouldStop{false};
            bool realTimeMonitoringEnabled{false};
            std::thread monitoringThread;
            std::function<void(const PerformanceMetrics &)> performanceCallback;
            mutable std::mutex mutex;
            std::condition_variable condition;

            // Timing tracking
            std::chrono::steady_clock::time_point startTime;

            // Cache analysis
            struct CacheSimulator {
                size_t hits{0};
                size_t misses{0};
                size_t lineSize{64};
                std::unordered_map<size_t, uint64> cacheLines;
            } cacheSimulator;
        };

// Helper macros for profiling
#if BUILD_DEBUG
#define PROFILE_ALLOCATION(ptr, size, tag) \
    Engine::Memory::MemoryProfiler::Get().OnAllocation(ptr, size, tag)
#define PROFILE_DEALLOCATION(ptr) \
    Engine::Memory::MemoryProfiler::Get().OnDeallocation(ptr)
#define PROFILE_MEMORY_ACCESS(ptr, size, isWrite) \
    Engine::Memory::MemoryProfiler::Get().OnMemoryAccess(ptr, size, isWrite)
#else
#define PROFILE_ALLOCATION(ptr, size, tag) ((void)0)
#define PROFILE_DEALLOCATION(ptr) ((void)0)
#define PROFILE_MEMORY_ACCESS(ptr, size, isWrite) ((void)0)
#endif

    }  // namespace Memory
}  // namespace Engine
