
#include "Memory/MemoryAnalyzer.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <numeric>

#include "Assert.h"
#include "Log/LogSystem.h"

namespace Engine {
    namespace Memory {

        MemoryAnalyzer &MemoryAnalyzer::Get() {
            static MemoryAnalyzer instance;
            return instance;
        }

        void MemoryAnalyzer::Initialize(const Config &config) {
            std::lock_guard<std::mutex> lock(mutex);
            this->config = config;

            allocations.clear();
            accessHistory.clear();
            history.clear();
            history.reserve(config.maxHistoryEntries);

            lastUpdate = std::chrono::steady_clock::now();
        }

        void MemoryAnalyzer::Shutdown() {
            std::lock_guard<std::mutex> lock(mutex);
            DetectMemoryLeaks();
            GenerateReport("memory_analysis_report.txt");
        }

        size_t MemoryAnalyzer::GetCurrentUsage() const {
            std::lock_guard<std::mutex> lock(mutex);
            return allocationMetrics.totalSize;
        }

        size_t MemoryAnalyzer::GetPeakUsage() const {
            std::lock_guard<std::mutex> lock(mutex);
            return allocationMetrics.maxSize;
        }

        void MemoryAnalyzer::OnAllocation(void *ptr,
                                          size_t size,
                                          const char *typeName,
                                          const MemoryLocation &location) {
            if (!ptr || !config.trackAllocationHistory) return;

            std::lock_guard<std::mutex> lock(mutex);

            auto start = std::chrono::steady_clock::now();

            AllocationInfo info;
            info.size = size;
            info.typeName = typeName;
            info.location = location;
            info.timestamp =
                std::chrono::steady_clock::now().time_since_epoch().count();

            allocations[ptr] = info;

            // Update metrics
            allocationMetrics.count++;
            allocationMetrics.totalSize += size;
            allocationMetrics.minSize =
                std::min(allocationMetrics.minSize, size);
            allocationMetrics.maxSize =
                std::max(allocationMetrics.maxSize, size);
            allocationMetrics.avgSize =
                static_cast<double>(allocationMetrics.totalSize) /
                allocationMetrics.count;

            // Update time metrics
            auto end = std::chrono::steady_clock::now();
            uint64 duration =
                std::chrono::duration_cast<std::chrono::nanoseconds>(end -
                                                                     start)
                    .count();

            info.allocationTime = duration;
            timeMetrics.totalAllocationTime += duration;
            timeMetrics.maxAllocationTime =
                std::max(timeMetrics.maxAllocationTime, duration);
            timeMetrics.avgAllocationTime =
                static_cast<double>(timeMetrics.totalAllocationTime) /
                allocationMetrics.count;

            if (duration > timeMetrics.avgAllocationTime * 2) {
                timeMetrics.slowAllocations++;
            }

            // Take snapshot if needed
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - lastUpdate)
                    .count() >= (long)config.sampleInterval) {
                TakeSnapshot();
                lastUpdate = now;
            }
        }

        void MemoryAnalyzer::OnDeallocation(
            void *ptr, [[maybe_unused]] const MemoryLocation &location) {
            if (!ptr) return;

            std::lock_guard<std::mutex> lock(mutex);

            auto start = std::chrono::steady_clock::now();

            auto it = allocations.find(ptr);
            if (it != allocations.end()) {
                // Update metrics
                allocationMetrics.totalSize -= it->second.size;
                allocations.erase(it);
            }

            // Update time metrics
            auto end = std::chrono::steady_clock::now();
            uint64 duration =
                std::chrono::duration_cast<std::chrono::nanoseconds>(end -
                                                                     start)
                    .count();

            timeMetrics.totalDeallocationTime += duration;
            timeMetrics.maxDeallocationTime =
                std::max(timeMetrics.maxDeallocationTime, duration);
            timeMetrics.avgDeallocationTime =
                static_cast<double>(timeMetrics.totalDeallocationTime) /
                allocationMetrics.count;

            if (duration > timeMetrics.avgDeallocationTime * 2) {
                timeMetrics.slowDeallocations++;
            }

            // Clean up access history
            if (config.trackAccessPatterns) {
                accessHistory.erase(ptr);
            }
        }

        void MemoryAnalyzer::OnAccess(void *ptr,
                                      size_t size,
                                      bool isWrite,
                                      const MemoryLocation &location) {
            if (!ptr || !config.trackAccessPatterns) return;

            std::lock_guard<std::mutex> lock(mutex);

            AccessInfo info;
            info.timestamp =
                std::chrono::steady_clock::now().time_since_epoch().count();
            info.isWrite = isWrite;
            info.size = size;
            info.location = location;

            auto &history = accessHistory[ptr];
            if (history.size() >= config.maxHistoryEntries) {
                history.erase(history.begin());
            }
            history.push_back(info);

            // Update access metrics
            if (isWrite) {
                accessMetrics.writeCount++;
            } else {
                accessMetrics.readCount++;
            }

            // Check if access is sequential
            if (!history.empty() && history.size() > 1) {
                const auto &lastAccess = history[history.size() - 2];
                if (static_cast<char *>(ptr) ==
                    static_cast<char *>(ptr) + lastAccess.size) {
                    accessMetrics.sequentialAccess++;
                } else {
                    accessMetrics.randomAccess++;
                }
            }
        }

        void MemoryAnalyzer::TakeSnapshot() {
            MemorySnapshot snapshot;
            snapshot.timestamp =
                std::chrono::steady_clock::now().time_since_epoch().count();
            snapshot.totalAllocated = allocationMetrics.totalSize;
            snapshot.totalFreed =
                allocationMetrics.totalSize - GetCurrentUsage();
            snapshot.currentUsage = GetCurrentUsage();
            snapshot.peakUsage = GetPeakUsage();
            snapshot.allocations = allocationMetrics.count;
            snapshot.deallocations =
                allocationMetrics.count - allocations.size();

            // Update fragmentation metrics
            UpdateFragmentationMetrics();
            snapshot.fragmentation = fragmentationMetrics;

            if (history.size() >= config.maxHistoryEntries) {
                history.erase(history.begin());
            }
            history.push_back(snapshot);
        }

        void MemoryAnalyzer::UpdateFragmentationMetrics() {
            fragmentationMetrics = FragmentationMetrics();

            if (allocations.empty()) return;

            // Sort allocations by address
            std::vector<std::pair<void *, size_t>> sorted;
            sorted.reserve(allocations.size());
            for (const auto &pair : allocations) {
                sorted.emplace_back(pair.first, pair.second.size);
            }
            std::sort(sorted.begin(), sorted.end());

            // Calculate fragments
            size_t totalGaps = 0;
            size_t maxGap = 0;
            size_t minGap = SIZE_MAX;

            for (size_t i = 1; i < sorted.size(); ++i) {
                size_t gap = reinterpret_cast<size_t>(sorted[i].first) -
                             (reinterpret_cast<size_t>(sorted[i - 1].first) +
                              sorted[i - 1].second);

                if (gap > 0) {
                    totalGaps += gap;
                    maxGap = std::max(maxGap, gap);
                    minGap = std::min(minGap, gap);
                    fragmentationMetrics.totalFragments++;
                }
            }

            fragmentationMetrics.largestFragment = maxGap;
            fragmentationMetrics.smallestFragment =
                minGap == SIZE_MAX ? 0 : minGap;
            fragmentationMetrics.wastedSpace = totalGaps;

            // Calculate fragmentation ratio
            size_t totalSpace = reinterpret_cast<size_t>(sorted.back().first) +
                                sorted.back().second -
                                reinterpret_cast<size_t>(sorted.front().first);

            fragmentationMetrics.fragmentation =
                totalSpace > 0 ? static_cast<double>(totalGaps) / totalSpace
                               : 0.0;

            fragmentationMetrics.utilization =
                totalSpace > 0 ? 1.0 - fragmentationMetrics.fragmentation : 1.0;
        }

        void MemoryAnalyzer::AnalyzeHotSpots() {
            if (!config.trackAccessPatterns) return;

            std::unordered_map<void *, size_t> accessCounts;

            // Count accesses for each memory location
            for (const auto &pair : accessHistory) {
                accessCounts[pair.first] = pair.second.size();
            }

            // Sort by access count
            std::vector<std::pair<void *, size_t>> sorted(accessCounts.begin(),
                                                          accessCounts.end());
            std::sort(
                sorted.begin(), sorted.end(), [](const auto &a, const auto &b) {
                    return a.second > b.second;
                });

            // Update hot spots (top 10)
            accessMetrics.hotSpots.clear();
            for (size_t i = 0; i < std::min(size_t(10), sorted.size()); ++i) {
                accessMetrics.hotSpots.push_back(
                    reinterpret_cast<size_t>(sorted[i].first));
            }
        }

        void MemoryAnalyzer::GenerateReport(const char *filename) const {
            std::ofstream file(filename);
            if (!file.is_open()) {
                LOG_ERROR("Failed to open file for memory analysis report: {}",
                          filename);
                return;
            }

            file << "Memory Analysis Report\n";
            file << "=====================\n\n";

            // Allocation metrics
            file << "Allocation Metrics:\n";
            file << "-----------------\n";
            file << "Total allocations: " << allocationMetrics.count << "\n";
            file << "Total size: " << allocationMetrics.totalSize << " bytes\n";
            file << "Average size: " << allocationMetrics.avgSize << " bytes\n";
            file << "Minimum size: " << allocationMetrics.minSize << " bytes\n";
            file << "Maximum size: " << allocationMetrics.maxSize << " bytes\n";
            file << "Allocation rate: " << allocationMetrics.allocRate
                 << "/s\n";
            file << "Deallocation rate: " << allocationMetrics.freeRate
                 << "/s\n\n";

            // Fragmentation metrics
            file << "Fragmentation Metrics:\n";
            file << "--------------------\n";
            file << "Total fragments: " << fragmentationMetrics.totalFragments
                 << "\n";
            file << "Largest fragment: " << fragmentationMetrics.largestFragment
                 << " bytes\n";
            file << "Smallest fragment: "
                 << fragmentationMetrics.smallestFragment << " bytes\n";
            file << "Fragmentation ratio: "
                 << (fragmentationMetrics.fragmentation * 100) << "%\n";
            file << "Memory utilization: "
                 << (fragmentationMetrics.utilization * 100) << "%\n";
            file << "Wasted space: " << fragmentationMetrics.wastedSpace
                 << " bytes\n\n";

            // Access metrics
            file << "Access Metrics:\n";
            file << "--------------\n";
            file << "Read operations: " << accessMetrics.readCount << "\n";
            file << "Write operations: " << accessMetrics.writeCount << "\n";
            file << "Sequential accesses: " << accessMetrics.sequentialAccess
                 << "\n";
            file << "Random accesses: " << accessMetrics.randomAccess << "\n";
            file << "Read/Write ratio: " << accessMetrics.readWriteRatio
                 << "\n\n";

            // Performance metrics
            file << "Performance Metrics:\n";
            file << "-------------------\n";
            file << "Average allocation time: " << timeMetrics.avgAllocationTime
                 << " ns\n";
            file << "Maximum allocation time: " << timeMetrics.maxAllocationTime
                 << " ns\n";
            file << "Average deallocation time: "
                 << timeMetrics.avgDeallocationTime << " ns\n";
            file << "Maximum deallocation time: "
                 << timeMetrics.maxDeallocationTime << " ns\n";
            file << "Slow allocations: " << timeMetrics.slowAllocations << "\n";
            file << "Slow deallocations: " << timeMetrics.slowDeallocations
                 << "\n\n";

            // Memory leaks
            file << "Memory Leaks:\n";
            file << "-------------\n";
            if (allocations.empty()) {
                file << "No memory leaks detected\n\n";
            } else {
                file << "Active allocations: " << allocations.size() << "\n";
                for (const auto &pair : allocations) {
                    const auto &info = pair.second;
                    file << "Address: " << pair.first << ", Size: " << info.size
                         << ", Type: "
                         << (info.typeName ? info.typeName : "Unknown")
                         << ", Location: " << info.location.file << ":"
                         << info.location.line << "\n";
                }
                file << "\n";
            }

            file.close();
        }

        void MemoryAnalyzer::DetectMemoryLeaks() const {
            if (allocations.empty()) {
                LOG_INFO("No memory leaks detected");
                return;
            }

            LOG_WARNING("Memory leaks detected:");
            for (const auto &pair : allocations) {
                const auto &info = pair.second;
                LOG_WARNING("Leak: {} bytes at {}, type: {}, location: {}:{}",
                            info.size,
                            pair.first,
                            info.typeName ? info.typeName : "Unknown",
                            info.location.file,
                            info.location.line);
            }
        }

        void MemoryAnalyzer::ResetStatistics() {
            std::lock_guard<std::mutex> lock(mutex);

            allocationMetrics = AllocationMetrics();
            fragmentationMetrics = FragmentationMetrics();
            accessMetrics = AccessMetrics();
            timeMetrics = TimeMetrics();

            history.clear();
            lastUpdate = std::chrono::steady_clock::now();
        }

        MemoryAnalyzer::AllocationMetrics MemoryAnalyzer::GetAllocationMetrics()
            const {
            std::lock_guard<std::mutex> lock(mutex);
            return allocationMetrics;
        }

        MemoryAnalyzer::FragmentationMetrics
        MemoryAnalyzer::GetFragmentationMetrics() const {
            std::lock_guard<std::mutex> lock(mutex);
            return fragmentationMetrics;
        }

        MemoryAnalyzer::AccessMetrics MemoryAnalyzer::GetAccessMetrics() const {
            std::lock_guard<std::mutex> lock(mutex);
            return accessMetrics;
        }

        MemoryAnalyzer::TimeMetrics MemoryAnalyzer::GetTimeMetrics() const {
            std::lock_guard<std::mutex> lock(mutex);
            return timeMetrics;
        }

        std::vector<MemoryAnalyzer::MemorySnapshot> MemoryAnalyzer::GetHistory()
            const {
            std::lock_guard<std::mutex> lock(mutex);
            return history;
        }

    }  // namespace Memory
}  // namespace Engine
