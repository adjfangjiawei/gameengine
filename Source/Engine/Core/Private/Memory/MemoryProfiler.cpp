
#include "Memory/MemoryProfiler.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <thread>

#include "Assert.h"
#include "Log/LogSystem.h"

#if PLATFORM_WINDOWS
#include <dbghelp.h>
#include <windows.h>
#pragma comment(lib, "dbghelp.lib")
#else
#include <execinfo.h>
#include <sys/resource.h>
#include <unistd.h>
#endif

namespace Engine {
    namespace Memory {

        // Fix the constructor to initialize all members declared in the header
        MemoryProfiler::PerformanceMetrics::PerformanceMetrics()
            : minAllocationTime(0),
              maxAllocationTime(0),
              totalAllocationTime(0),
              averageAllocationTime(0.0),
              allocationLatencyP95(0.0),
              allocationLatencyP99(0.0),
              allocationsPerSecond(0.0),
              deallocationsPerSecond(0.0),
              bytesPerSecond(0.0),
              cacheHitRate(0.0),
              cacheMissRate(0.0),
              cacheLineTransfers(0),
              pageFaults(0),
              systemCalls(0),
              lockContentionRate(0.0) {}

        MemoryProfiler &MemoryProfiler::Get() {
            static MemoryProfiler instance;
            return instance;
        }

        void MemoryProfiler::Initialize(const Config &config) {
            std::lock_guard<std::mutex> lock(mutex);
            this->config = config;

            allocations.clear();
            threadContexts.clear();
            globalSamples.clear();
            hotspots.clear();

            currentMetrics = MemoryProfiler::PerformanceMetrics();

            // Reset atomic counters
            totalAllocations = 0;
            totalDeallocations = 0;
            totalAllocationTime = 0;
            totalDeallocationTime = 0;
            pageFaultCount = 0;
            systemCallCount = 0;
            lockContentionCount = 0;

            // Initialize cache simulator
            cacheSimulator = CacheSimulator();

            if (config.enableDetailedTracking) {
                StartMonitoringThread();
            }
        }

        void MemoryProfiler::Shutdown() {
            StopProfiling();
            std::lock_guard<std::mutex> lock(mutex);
            GenerateReport("memory_profile_final.txt");
        }

        void MemoryProfiler::StartProfiling() {
            std::lock_guard<std::mutex> lock(mutex);
            if (!isRunning) {
                isRunning = true;
                isPaused = false;
                shouldStop = false;
                if (config.enableDetailedTracking) {
                    StartMonitoringThread();
                }
            }
        }

        void MemoryProfiler::StopProfiling() {
            {
                std::lock_guard<std::mutex> lock(mutex);
                if (!isRunning) return;
                shouldStop = true;
            }
            condition.notify_one();

            if (monitoringThread.joinable()) {
                monitoringThread.join();
            }

            {
                std::lock_guard<std::mutex> lock(mutex);
                isRunning = false;
                isPaused = false;
            }
        }

        void MemoryProfiler::PauseProfiling() {
            std::lock_guard<std::mutex> lock(mutex);
            isPaused = true;
        }

        void MemoryProfiler::ResumeProfiling() {
            {
                std::lock_guard<std::mutex> lock(mutex);
                isPaused = false;
            }
            condition.notify_one();
        }

        bool MemoryProfiler::IsProfiling() const {
            std::lock_guard<std::mutex> lock(mutex);
            return isRunning && !isPaused;
        }

        void MemoryProfiler::OnAllocation(void *ptr,
                                          size_t size,
                                          const char *tag) {
            if (!ptr || !isRunning || isPaused) return;

            auto start = std::chrono::steady_clock::now();

            {
                std::lock_guard<std::mutex> lock(mutex);

                AllocationProfile profile;
                profile.size = size;
                profile.timestamp =
                    std::chrono::steady_clock::now().time_since_epoch().count();
                profile.threadId = static_cast<uint32>(
                    std::hash<std::thread::id>{}(std::this_thread::get_id()));
                profile.tag = tag;

                if (config.enableCallstackTracking) {
                    CaptureCallstack(profile.callstack, profile.callstackDepth);
                }

                allocations[ptr] = profile;

                // Update thread stats
                auto &context = threadContexts[profile.threadId];
                context.stats.totalAllocations++;
                context.stats.currentBytes += size;
                context.stats.peakBytes = std::max(context.stats.peakBytes,
                                                   context.stats.currentBytes);

                // Add sample
                Sample sample;
                sample.timestamp = profile.timestamp;
                sample.allocatedBytes = size;
                sample.deallocatedBytes = 0;
                sample.threadId = profile.threadId;

                if (context.samples.size() >= config.maxSamples) {
                    context.samples.erase(context.samples.begin());
                }
                context.samples.push_back(sample);
            }

            auto end = std::chrono::steady_clock::now();
            uint64 duration =
                std::chrono::duration_cast<std::chrono::nanoseconds>(end -
                                                                     start)
                    .count();

            totalAllocations.fetch_add(1);
            totalAllocationTime.fetch_add(duration);
        }

        void MemoryProfiler::OnDeallocation(void *ptr) {
            if (!ptr || !isRunning || isPaused) return;

            auto start = std::chrono::steady_clock::now();

            {
                std::lock_guard<std::mutex> lock(mutex);

                auto it = allocations.find(ptr);
                if (it != allocations.end()) {
                    const auto &profile = it->second;

                    // Update thread stats
                    auto &context = threadContexts[profile.threadId];
                    context.stats.totalDeallocations++;
                    context.stats.currentBytes -= profile.size;

                    // Add sample
                    Sample sample;
                    sample.timestamp = std::chrono::steady_clock::now()
                                           .time_since_epoch()
                                           .count();
                    sample.allocatedBytes = 0;
                    sample.deallocatedBytes = profile.size;
                    sample.threadId = profile.threadId;

                    if (context.samples.size() >= config.maxSamples) {
                        context.samples.erase(context.samples.begin());
                    }
                    context.samples.push_back(sample);

                    allocations.erase(it);
                }
            }

            auto end = std::chrono::steady_clock::now();
            uint64 duration =
                std::chrono::duration_cast<std::chrono::nanoseconds>(end -
                                                                     start)
                    .count();

            totalDeallocations.fetch_add(1);
            totalDeallocationTime.fetch_add(duration);
        }

        void MemoryProfiler::OnMemoryAccess(void *ptr,
                                            size_t size,
                                            bool isWrite) {
            if (!ptr || !isRunning || isPaused ||
                !config.enableHotspotDetection)
                return;

            std::lock_guard<std::mutex> lock(mutex);

            // Update cache simulator
            size_t lineAddress =
                reinterpret_cast<size_t>(ptr) / cacheSimulator.lineSize;
            if (cacheSimulator.cacheLines.find(lineAddress) !=
                cacheSimulator.cacheLines.end()) {
                cacheSimulator.hits++;
            } else {
                cacheSimulator.misses++;
                cacheSimulator.cacheLines[lineAddress] =
                    std::chrono::steady_clock::now().time_since_epoch().count();
            }

            // Update hotspot tracking
            for (auto &hotspot : hotspots) {
                if (hotspot.address == ptr) {
                    hotspot.accessCount++;
                    hotspot.totalBytes += size;
                    uint32 threadId =
                        static_cast<uint32>(std::hash<std::thread::id>{}(
                            std::this_thread::get_id()));
                    if (std::find(hotspot.threadIds.begin(),
                                  hotspot.threadIds.end(),
                                  threadId) == hotspot.threadIds.end()) {
                        hotspot.threadIds.push_back(threadId);
                    }
                    return;
                }
            }

            // Create new hotspot if threshold reached
            if (hotspots.size() < config.hotspotThreshold) {
                HotspotInfo info;
                info.address = ptr;
                info.accessCount = 1;
                info.totalBytes = size;
                info.isWrite = isWrite;
                info.threadIds.push_back(static_cast<uint32>(
                    std::hash<std::thread::id>{}(std::this_thread::get_id())));
                hotspots.push_back(info);
            }
        }

        MemoryProfiler::PerformanceMetrics
        MemoryProfiler::GetPerformanceMetrics() const {
            std::lock_guard<std::mutex> lock(mutex);

            PerformanceMetrics metrics = currentMetrics;

            // Calculate current rates
            auto now = std::chrono::steady_clock::now();
            double elapsed =
                std::chrono::duration<double>(now - startTime).count();

            if (elapsed > 0) {
                metrics.allocationsPerSecond =
                    static_cast<double>(totalAllocations) / elapsed;
                metrics.deallocationsPerSecond =
                    static_cast<double>(totalDeallocations) / elapsed;
            }

            // Calculate cache metrics
            size_t totalAccesses = cacheSimulator.hits + cacheSimulator.misses;
            if (totalAccesses > 0) {
                metrics.cacheHitRate =
                    static_cast<double>(cacheSimulator.hits) / totalAccesses;
                metrics.cacheMissRate =
                    static_cast<double>(cacheSimulator.misses) / totalAccesses;
            }

            // Update system metrics
            metrics.lockContentionRate = CalculateLockContention();

            return metrics;
        }

        std::vector<MemoryProfiler::HotspotInfo> MemoryProfiler::GetHotspots()
            const {
            std::lock_guard<std::mutex> lock(mutex);

            // Sort hotspots by access count
            std::vector<HotspotInfo> sortedHotspots = hotspots;
            std::sort(sortedHotspots.begin(),
                      sortedHotspots.end(),
                      [](const HotspotInfo &a, const HotspotInfo &b) {
                          return a.accessCount > b.accessCount;
                      });

            return sortedHotspots;
        }

        MemoryProfiler::ThreadStats MemoryProfiler::GetThreadStats(
            uint32 threadId) const {
            std::lock_guard<std::mutex> lock(mutex);

            auto it = threadContexts.find(threadId);
            if (it != threadContexts.end()) {
                return it->second.stats;
            }

            return ThreadStats();
        }

        std::vector<MemoryProfiler::AllocationProfile>
        MemoryProfiler::GetAllocationHistory() const {
            std::lock_guard<std::mutex> lock(mutex);

            std::vector<AllocationProfile> history;
            history.reserve(allocations.size());

            for (const auto &pair : allocations) {
                history.push_back(pair.second);
            }

            // Sort by timestamp
            std::sort(
                history.begin(),
                history.end(),
                [](const AllocationProfile &a, const AllocationProfile &b) {
                    return a.timestamp < b.timestamp;
                });

            return history;
        }

        void MemoryProfiler::GenerateReport(const char *filename) const {
            std::ofstream file(filename);
            if (!file.is_open()) {
                LOG_ERROR("Failed to open memory profile report file: {}",
                          filename);
                return;
            }

            file << "Memory Performance Profile Report\n";
            file << "==============================\n\n";

            // Performance metrics
            auto metrics = GetPerformanceMetrics();
            file << "Performance Metrics:\n";
            file << "------------------\n";
            file << "Allocation Rate: " << std::fixed << std::setprecision(2)
                 << metrics.allocationsPerSecond << "/s\n";
            file << "Deallocation Rate: " << metrics.deallocationsPerSecond
                 << "/s\n";
            file << "Average Allocation Time: " << metrics.averageAllocationTime
                 << " ns\n";
            file << "95th Percentile Latency: " << metrics.allocationLatencyP95
                 << " ns\n";
            file << "99th Percentile Latency: " << metrics.allocationLatencyP99
                 << " ns\n";
            file << "Cache Hit Rate: " << (metrics.cacheHitRate * 100) << "%\n";
            file << "Lock Contention Rate: "
                 << (metrics.lockContentionRate * 100) << "%\n\n";

            // Thread statistics
            file << "Thread Statistics:\n";
            file << "-----------------\n";
            for (const auto &pair : threadContexts) {
                const auto &stats = pair.second.stats;
                file << "Thread " << pair.first << ":\n";
                file << "  Allocations: " << stats.totalAllocations << "\n";
                file << "  Deallocations: " << stats.totalDeallocations << "\n";
                file << "  Current Memory: " << stats.currentBytes
                     << " bytes\n";
                file << "  Peak Memory: " << stats.peakBytes << " bytes\n";
                file << "  Average Allocation Time: "
                     << stats.averageAllocationTime << " ns\n\n";
            }

            // Hotspots
            file << "Memory Hotspots:\n";
            file << "---------------\n";
            auto hotspots = GetHotspots();
            for (const auto &hotspot : hotspots) {
                file << "Address: " << hotspot.address << "\n";
                file << "  Access Count: " << hotspot.accessCount << "\n";
                file << "  Total Bytes: " << hotspot.totalBytes << "\n";
                file << "  Operation: " << (hotspot.isWrite ? "Write" : "Read")
                     << "\n";
                file << "  Threads: ";
                for (uint32 threadId : hotspot.threadIds) {
                    file << threadId << " ";
                }
                file << "\n\n";
            }

            // Allocation patterns
            file << "Allocation Patterns:\n";
            file << "-------------------\n";
            auto history = GetAllocationHistory();
            std::unordered_map<size_t, size_t> sizeCounts;
            for (const auto &profile : history) {
                sizeCounts[profile.size]++;
            }

            std::vector<std::pair<size_t, size_t>> sortedSizes(
                sizeCounts.begin(), sizeCounts.end());
            std::sort(sortedSizes.begin(),
                      sortedSizes.end(),
                      [](const auto &a, const auto &b) {
                          return a.second > b.second;
                      });

            for (const auto &pair : sortedSizes) {
                file << "Size " << pair.first << " bytes: " << pair.second
                     << " allocations\n";
            }

            file.close();
        }

        void MemoryProfiler::StartMonitoringThread() {
            if (monitoringThread.joinable()) {
                monitoringThread.join();
            }

            shouldStop = false;
            startTime = std::chrono::steady_clock::now();
            monitoringThread =
                std::thread(&MemoryProfiler::MonitoringThreadFunction, this);
        }

        void MemoryProfiler::MonitoringThreadFunction() {
            while (!shouldStop) {
                {
                    std::unique_lock<std::mutex> lock(mutex);
                    condition.wait_for(
                        lock,
                        std::chrono::milliseconds(config.sampleInterval),
                        [this]() { return shouldStop || !isPaused; });

                    if (shouldStop) break;
                    if (isPaused) continue;

                    UpdateMetrics();
                }

                if (realTimeMonitoringEnabled && performanceCallback) {
                    performanceCallback(currentMetrics);
                }
            }
        }

        void MemoryProfiler::UpdateMetrics() {
            // Update timing metrics
            size_t totalAllocs = totalAllocations.load();
            if (totalAllocs > 0) {
                currentMetrics.averageAllocationTime =
                    static_cast<double>(totalAllocationTime) / totalAllocs;
            }

            // Calculate latency percentiles
            CalculateLatencyPercentiles();

            // Update cache metrics
            size_t totalAccesses = cacheSimulator.hits + cacheSimulator.misses;
            if (totalAccesses > 0) {
                currentMetrics.cacheHitRate =
                    static_cast<double>(cacheSimulator.hits) / totalAccesses;
                currentMetrics.cacheMissRate =
                    static_cast<double>(cacheSimulator.misses) / totalAccesses;
            }

            // Update system metrics
            currentMetrics.pageFaults = pageFaultCount.load();
            currentMetrics.systemCalls = systemCallCount.load();
            currentMetrics.lockContentionRate = CalculateLockContention();

            // Detect hotspots
            if (config.enableHotspotDetection) {
                DetectHotspots();
            }
        }

        void MemoryProfiler::CalculateLatencyPercentiles() {
            std::vector<uint64> latencies;
            latencies.reserve(globalSamples.size());

            for (const auto &sample : globalSamples) {
                latencies.push_back(sample.allocationTime);
            }

            if (!latencies.empty()) {
                std::sort(latencies.begin(), latencies.end());
                size_t p95Index = static_cast<size_t>(latencies.size() * 0.95);
                size_t p99Index = static_cast<size_t>(latencies.size() * 0.99);

                currentMetrics.allocationLatencyP95 = latencies[p95Index];
                currentMetrics.allocationLatencyP99 = latencies[p99Index];
            }
        }

        void MemoryProfiler::DetectHotspots() {
            // Sort hotspots by access count and keep only the top ones
            if (hotspots.size() > config.hotspotThreshold) {
                std::sort(hotspots.begin(),
                          hotspots.end(),
                          [](const HotspotInfo &a, const HotspotInfo &b) {
                              return a.accessCount > b.accessCount;
                          });
                hotspots.resize(config.hotspotThreshold);
            }
        }

        double MemoryProfiler::CalculateLockContention() const {
            size_t contentions = lockContentionCount.load();
            size_t totalOps =
                totalAllocations.load() + totalDeallocations.load();

            return totalOps > 0 ? static_cast<double>(contentions) / totalOps
                                : 0.0;
        }

        void MemoryProfiler::CaptureCallstack(void **callstack, size_t &depth) {
#if PLATFORM_WINDOWS
            depth = CaptureStackBackTrace(1, 32, callstack, nullptr);
#else
            depth = backtrace(callstack, 32);
#endif
        }

        void MemoryProfiler::EnableRealTimeMonitoring(bool enable) {
            realTimeMonitoringEnabled = enable;
        }

        void MemoryProfiler::SetPerformanceCallback(
            std::function<void(const PerformanceMetrics &)> callback) {
            performanceCallback = callback;
        }

        void MemoryProfiler::DumpStatistics() const {
            std::lock_guard<std::mutex> lock(mutex);

            LOG_INFO("Memory Profiler Statistics");
            LOG_INFO("=========================");

            // Performance metrics
            auto metrics = GetPerformanceMetrics();
            LOG_INFO("Allocation Rate: {:.2f}/s", metrics.allocationsPerSecond);
            LOG_INFO("Deallocation Rate: {:.2f}/s",
                     metrics.deallocationsPerSecond);
            LOG_INFO("Average Allocation Time: {:.2f} ns",
                     metrics.averageAllocationTime);
            LOG_INFO("95th Percentile Latency: {:.2f} ns",
                     metrics.allocationLatencyP95);
            LOG_INFO("99th Percentile Latency: {:.2f} ns",
                     metrics.allocationLatencyP99);
            LOG_INFO("Cache Hit Rate: {:.2f}%", metrics.cacheHitRate * 100);
            LOG_INFO("Lock Contention Rate: {:.2f}%",
                     metrics.lockContentionRate * 100);

            // Total statistics
            LOG_INFO("Total Allocations: {}", totalAllocations.load());
            LOG_INFO("Total Deallocations: {}", totalDeallocations.load());
            LOG_INFO("Current Allocation Count: {}", allocations.size());

            // Thread statistics
            LOG_INFO("Per-Thread Statistics:");
            for (const auto &pair : threadContexts) {
                const auto &stats = pair.second.stats;
                LOG_INFO(
                    "  Thread {}: Allocs={}, Deallocs={}, Current={}B, "
                    "Peak={}B",
                    pair.first,
                    stats.totalAllocations,
                    stats.totalDeallocations,
                    stats.currentBytes,
                    stats.peakBytes);
            }

            // Hotspots (if enabled and if we have any)
            if (config.enableHotspotDetection && !hotspots.empty()) {
                LOG_INFO("Top Memory Hotspots:");
                auto sortedHotspots = GetHotspots();
                size_t count =
                    std::min(sortedHotspots.size(), static_cast<size_t>(5));
                for (size_t i = 0; i < count; i++) {
                    const auto &hotspot = sortedHotspots[i];
                    LOG_INFO("  Address: {}, Accesses: {}, Bytes: {}",
                             hotspot.address,
                             hotspot.accessCount,
                             hotspot.totalBytes);
                }
            }
        }

    }  // namespace Memory
}  // namespace Engine
