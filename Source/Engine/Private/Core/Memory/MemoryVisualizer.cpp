
#include "Core/Memory/MemoryVisualizer.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>

#include "Core/Assert.h"
#include "Core/Log/LogSystem.h"

namespace Engine {
    namespace Memory {

        MemoryVisualizer &MemoryVisualizer::Get() {
            static MemoryVisualizer instance;
            return instance;
        }

        void MemoryVisualizer::Initialize(const Config &config) {
            std::lock_guard<std::mutex> lock(mutex);
            this->config = config;

            // Initialize visualization data
            data.timeSeries.clear();
            data.timeSeries.reserve(config.historyLength);

            // Initialize heatmap with empty rows
            std::vector<float> row(64, 0.0f);  // 64 columns
            data.heatmap.clear();
            data.heatmap.resize(64, row);  // 64 rows

            data.lastUpdate =
                std::chrono::steady_clock::now().time_since_epoch().count();

            if (config.enableRealTimeUpdates) {
                StartVisualization();
            }
        }

        void MemoryVisualizer::Shutdown() {
            StopVisualization();
            std::lock_guard<std::mutex> lock(mutex);

            if (config.enableDataExport) {
                ExportToJSON("memory_visualization_final.json");
            }
        }

        void MemoryVisualizer::StartVisualization() {
            std::lock_guard<std::mutex> lock(mutex);
            if (!isRunning) {
                isRunning = true;
                isPaused = false;
                updateThread = std::thread([this]() {
                    while (isRunning) {
                        {
                            std::unique_lock<std::mutex> lock(mutex);
                            condition.wait_for(
                                lock,
                                std::chrono::milliseconds(
                                    config.updateInterval),
                                [this]() { return !isRunning || !isPaused; });

                            if (!isRunning) break;
                            if (isPaused) continue;

                            UpdateVisualization();
                        }
                        NotifyUpdateCallbacks();
                    }
                });
            }
        }

        void MemoryVisualizer::StopVisualization() {
            {
                std::lock_guard<std::mutex> lock(mutex);
                if (!isRunning) return;
                isRunning = false;
            }
            condition.notify_one();

            if (updateThread.joinable()) {
                updateThread.join();
            }
        }

        void MemoryVisualizer::PauseVisualization() {
            std::lock_guard<std::mutex> lock(mutex);
            isPaused = true;
        }

        void MemoryVisualizer::ResumeVisualization() {
            {
                std::lock_guard<std::mutex> lock(mutex);
                isPaused = false;
            }
            condition.notify_one();
        }

        bool MemoryVisualizer::IsVisualizing() const {
            std::lock_guard<std::mutex> lock(mutex);
            return isRunning && !isPaused;
        }

        void MemoryVisualizer::OnMemoryAllocation(
            void *ptr,
            size_t size,
            [[maybe_unused]] uint32 poolIndex,
            [[maybe_unused]] const char *tag) {
            if (!ptr || !isRunning) return;

            std::lock_guard<std::mutex> lock(mutex);

            MemoryAccess access;
            access.address = ptr;
            access.size = size;
            access.isWrite = true;
            access.timestamp = static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch())
                    .count());

            ProcessMemoryAccess(access);
            UpdateHeatmap(ptr, size, true);
        }

        void MemoryVisualizer::OnMemoryDeallocation(
            void *ptr, [[maybe_unused]] uint32 poolIndex) {
            if (!ptr || !isRunning) return;

            std::lock_guard<std::mutex> lock(mutex);

            MemoryAccess access;
            access.address = ptr;
            access.size = 0;  // Size is unknown at deallocation
            access.isWrite = true;
            access.timestamp = static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch())
                    .count());

            ProcessMemoryAccess(access);
            UpdateHeatmap(ptr, 0, true);
        }

        void MemoryVisualizer::OnMemoryAccess(void *ptr,
                                              size_t size,
                                              bool isWrite) {
            if (!ptr || !isRunning || !config.enableRealTimeUpdates) return;

            std::lock_guard<std::mutex> lock(mutex);
            UpdateHeatmap(ptr, size, isWrite);
        }

        void MemoryVisualizer::GenerateMemoryMap(
            const std::string &filename) const {
            std::lock_guard<std::mutex> lock(mutex);
            ExportToJSON(filename);
        }

        void MemoryVisualizer::GenerateHeatmap(
            const std::string &filename) const {
            std::lock_guard<std::mutex> lock(mutex);
            ExportToJSON(filename);
        }

        void MemoryVisualizer::GenerateTimeSeries(
            const std::string &filename) const {
            std::lock_guard<std::mutex> lock(mutex);
            ExportToCSV(filename);
        }

        void MemoryVisualizer::ExportToJSON(const std::string &filename) const {
            std::lock_guard<std::mutex> lock(mutex);

            std::ofstream file(filename);
            if (!file.is_open()) return;

            file << "{\n";

            // Export time series
            file << "  \"timeSeries\": [\n";
            for (const auto &point : data.timeSeries) {
                file << "    {\n";
                file << "      \"timestamp\": " << point.timestamp << ",\n";
                file << "      \"value\": " << point.value << "\n";
                file << "    }";
                if (&point != &data.timeSeries.back()) file << ",";
                file << "\n";
            }
            file << "  ],\n";

            // Export heatmap
            file << "  \"heatmap\": [\n";
            for (size_t y = 0; y < data.heatmap.size(); ++y) {
                file << "    [";
                for (size_t x = 0; x < data.heatmap[y].size(); ++x) {
                    file << data.heatmap[y][x];
                    if (x < data.heatmap[y].size() - 1) file << ", ";
                }
                file << "]";
                if (y < data.heatmap.size() - 1) file << ",";
                file << "\n";
            }
            file << "  ]\n";

            file << "}\n";
        }

        void MemoryVisualizer::ExportToCSV(const std::string &filename) const {
            std::lock_guard<std::mutex> lock(mutex);

            std::ofstream file(filename);
            if (!file.is_open()) return;

            // Write time series data
            file << "Timestamp,Value\n";

            for (const auto &point : data.timeSeries) {
                file << point.timestamp << "," << point.value << "\n";
            }
        }

        void MemoryVisualizer::UpdateVisualization() {
            // Add new time series point
            AddTimeSeriesPoint();

            // Cleanup old data if needed
            CleanupOldData();

            // Update last update time
            data.lastUpdate =
                std::chrono::steady_clock::now().time_since_epoch().count();

            // Notify any registered callbacks
            NotifyUpdateCallbacks();
        }

        void MemoryVisualizer::ProcessMemoryAccess(const MemoryAccess &access) {
            // Find or create appropriate region
            for (auto &region : data.regions) {
                if (access.address >= region.start &&
                    static_cast<char *>(access.address) + access.size <=
                        static_cast<char *>(region.start) + region.size) {
                    return;
                }
            }

            // Create new region
            MemoryRegion region;
            region.start = access.address;
            region.size = access.size;
            region.flags = 0;
            data.regions.push_back(region);
        }

        void MemoryVisualizer::UpdateHeatmap(void *ptr,
                                             size_t size,
                                             bool isWrite) {
            if (!ptr || size == 0) return;

            // Create a simple intensity map based on access frequency
            std::vector<float> row(64, 0.0f);  // 64 columns for visualization
            if (data.heatmap.empty()) {
                data.heatmap.resize(64, row);  // 64x64 grid for visualization
            }

            // Update intensity at the corresponding position
            size_t x = (reinterpret_cast<size_t>(ptr) / 4096) % 64;
            size_t y = (size / 64) % 64;

            float intensity = isWrite ? 1.0f : 0.5f;
            data.heatmap[y][x] = std::min(1.0f, data.heatmap[y][x] + intensity);
        }

        void MemoryVisualizer::AddTimeSeriesPoint() {
            TimeSeriesData point;
            point.timestamp =
                std::chrono::steady_clock::now().time_since_epoch().count();

            // Calculate total memory usage
            size_t totalMemory = 0;
            for (const auto &region : data.regions) {
                totalMemory += region.size;
            }
            point.value = totalMemory;

            data.timeSeries.push_back(point);

            // Limit the size of time series data
            while (data.timeSeries.size() > config.historyLength) {
                data.timeSeries.erase(data.timeSeries.begin());
            }
        }

        void MemoryVisualizer::CleanupOldData() {
            // Remove old time series points
            while (data.timeSeries.size() > config.historyLength) {
                data.timeSeries.erase(data.timeSeries.begin());
            }

            // Decay heatmap values
            for (auto &row : data.heatmap) {
                for (auto &value : row) {
                    value *= 0.99f;  // Gradual decay of intensity
                    if (value < 0.01f) {
                        value = 0.0f;
                    }
                }
            }
        }

        void MemoryVisualizer::MergeAdjacentRegions() {
            if (data.regions.size() < 2) return;

            std::sort(data.regions.begin(),
                      data.regions.end(),
                      [](const MemoryRegion &a, const MemoryRegion &b) {
                          return a.start < b.start;
                      });

            for (size_t i = 0; i < data.regions.size() - 1;) {
                auto &current = data.regions[i];
                auto &next = data.regions[i + 1];

                if (static_cast<char *>(current.start) + current.size ==
                    next.start) {
                    // Merge regions
                    current.size += next.size;
                    current.flags |= next.flags;
                    data.regions.erase(data.regions.begin() + i + 1);
                } else {
                    ++i;
                }
            }
        }

        void MemoryVisualizer::CompressTimeSeriesData() {
            if (data.timeSeries.size() <= config.maxDataPoints) return;

            // Compress by averaging adjacent points
            size_t compressionFactor = 2;
            std::vector<TimeSeriesData> compressed;
            compressed.reserve(data.timeSeries.size() / compressionFactor);

            for (size_t i = 0; i < data.timeSeries.size();
                 i += compressionFactor) {
                TimeSeriesData avg;
                size_t count = 0;
                size_t totalValue = 0;

                for (size_t j = 0;
                     j < compressionFactor && i + j < data.timeSeries.size();
                     ++j) {
                    const auto &point = data.timeSeries[i + j];
                    totalValue += point.value;
                    count++;
                }

                if (count > 0) {
                    avg.timestamp = data.timeSeries[i].timestamp;
                    avg.value = totalValue / count;
                    compressed.push_back(avg);
                }
            }

            data.timeSeries = std::move(compressed);
        }

        float MemoryVisualizer::CalculateFragmentation() const {
            if (data.regions.empty()) {
                return 0.0f;
            }

            size_t totalSize = 0;
            size_t gaps = 0;
            void *lastEnd = nullptr;

            for (const auto &region : data.regions) {
                totalSize += region.size;
                if (lastEnd) {
                    gaps += static_cast<char *>(region.start) -
                            static_cast<char *>(lastEnd);
                }
                lastEnd = static_cast<char *>(region.start) + region.size;
            }

            return totalSize > 0 ? static_cast<float>(gaps) / totalSize : 0.0f;
        }

        void MemoryVisualizer::NotifyUpdateCallbacks() {
            if (updateCallback) {
                updateCallback(data);
            }
        }

        void MemoryVisualizer::SetUpdateCallback(
            std::function<void(const VisualizationData &)> callback) {
            std::lock_guard<std::mutex> lock(mutex);
            updateCallback = callback;
        }

        void MemoryVisualizer::OnPoolCreated(
            uint32_t poolIndex, [[maybe_unused]] size_t blockSize) {
            std::lock_guard<std::mutex> lock(mutex);
            data.poolUsage[poolIndex] = 0;
        }

        void MemoryVisualizer::OnPoolDestroyed(uint32_t poolIndex) {
            std::lock_guard<std::mutex> lock(mutex);
            data.poolUsage.erase(poolIndex);
        }

        void MemoryVisualizer::ExportToFile(const std::string &filename) const {
            std::lock_guard<std::mutex> lock(mutex);

            // Export both JSON and CSV data
            ExportToJSON(filename + ".json");
            ExportToCSV(filename + ".csv");
        }

        void MemoryVisualizer::GenerateReport(
            const std::string &filename) const {
            std::lock_guard<std::mutex> lock(mutex);
            std::ofstream file(filename);
            if (!file.is_open()) return;

            // Write report header
            file << "Memory Visualization Report\n";
            file << "========================\n\n";

            // Memory statistics
            file << "Memory Statistics:\n";
            file << "-----------------\n";
            size_t totalMemory = 0;
            for (const auto &region : data.regions) {
                totalMemory += region.size;
            }
            file << "Total Memory: " << totalMemory << " bytes\n";
            file << "Fragmentation: " << (CalculateFragmentation() * 100.0f)
                 << "%\n\n";

            // Pool usage
            file << "Memory Pool Usage:\n";
            file << "----------------\n";
            for (const auto &pool : data.poolUsage) {
                file << "Pool " << pool.first << ": " << pool.second
                     << " bytes\n";
            }
            file << "\n";

            // Recent activity
            file << "Recent Memory Activity:\n";
            file << "--------------------\n";
            size_t recentActivityCount =
                std::min(size_t(10), data.accesses.size());
            for (size_t i = data.accesses.size() - recentActivityCount;
                 i < data.accesses.size();
                 ++i) {
                const auto &access = data.accesses[i];
                file << (access.isWrite ? "Write" : "Read") << " at "
                     << access.address << " size: " << access.size
                     << " bytes\n";
            }
        }

        void MemoryVisualizer::DumpStats() const {
            std::lock_guard<std::mutex> lock(mutex);

            LOG_INFO("Memory Visualizer Statistics:");
            LOG_INFO("---------------------------");

            // Calculate basic statistics
            size_t totalMemory = 0;
            for (const auto &region : data.regions) {
                totalMemory += region.size;
            }

            LOG_INFO("Total Memory: {} bytes", totalMemory);
            LOG_INFO("Active Regions: {}", data.regions.size());
            LOG_INFO("Memory Pools: {}", data.poolUsage.size());
            LOG_INFO("Fragmentation: {:.2f}%",
                     CalculateFragmentation() * 100.0f);
            LOG_INFO("Time Series Points: {}", data.timeSeries.size());
        }

        void MemoryVisualizer::TakeSnapshot() {
            std::lock_guard<std::mutex> lock(mutex);

            MemorySnapshot snapshot;
            snapshot.timestamp =
                std::chrono::steady_clock::now().time_since_epoch().count();

            // Calculate memory statistics
            snapshot.totalMemory = 0;
            snapshot.usedMemory = 0;
            snapshot.fragmentedMemory = 0;

            for (const auto &block : activeBlocks) {
                snapshot.blocks.push_back(block.second);
                snapshot.usedMemory += block.second.size;
            }

            for (const auto &region : data.regions) {
                snapshot.totalMemory += region.size;
            }

            snapshot.fragmentedMemory = static_cast<size_t>(
                snapshot.totalMemory * CalculateFragmentation());
            snapshot.peakMemory = std::max(
                snapshot.totalMemory,
                data.history.empty() ? 0 : data.history.back().peakMemory);

            data.history.push_back(snapshot);

            // Limit history size
            while (data.history.size() > config.historyLength) {
                data.history.erase(data.history.begin());
            }
        }

        const MemoryVisualizer::VisualizationData &
        MemoryVisualizer::GetCurrentData() const {
            std::lock_guard<std::mutex> lock(mutex);
            return data;
        }

    }  // namespace Memory
}  // namespace Engine
