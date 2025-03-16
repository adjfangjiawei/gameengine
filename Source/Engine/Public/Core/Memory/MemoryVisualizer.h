
#pragma once

#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "Core/CoreTypes.h"
#include "Core/Memory/MemoryTypes.h"

namespace Engine {
    namespace Memory {

        class MemoryVisualizer {
          public:
            struct Config {
                size_t updateInterval{100};  // Update interval in milliseconds
                size_t historyLength{1000};  // Number of history points to keep
                size_t maxDataPoints{10000};  // Maximum number of data points
                bool enableRealTimeUpdates{
                    true};  // Enable real-time visualization updates
                bool enableInteractiveMode{
                    true};  // Enable interactive visualization mode
                bool enableDataExport{
                    true};               // Enable data export functionality
                std::string outputPath;  // Path for exported data files
            };

            struct MemoryBlock {
                void *address;       // Block address
                size_t size;         // Block size
                uint32_t poolIndex;  // Pool index
                const char *tag;     // Optional tag
                uint64_t timestamp;  // Allocation timestamp
            };

            struct MemoryAccess {
                void *address;       // Access address
                size_t size;         // Access size
                bool isWrite;        // Whether it's a write access
                uint64_t timestamp;  // Access timestamp
            };

            struct MemorySnapshot {
                uint64_t timestamp;               // Snapshot timestamp
                size_t totalMemory;               // Total memory allocated
                size_t usedMemory;                // Memory in use
                size_t peakMemory;                // Peak memory usage
                size_t fragmentedMemory;          // Fragmented memory
                std::vector<MemoryBlock> blocks;  // Active memory blocks
            };

            struct MemoryRegion {
                void *start;
                size_t size;
                uint32_t flags;
            };

            struct TimeSeriesData {
                uint64_t timestamp;
                size_t value;
            };

            struct VisualizationData {
                std::vector<MemoryRegion> regions;    // Memory regions
                std::vector<MemorySnapshot> history;  // Memory usage history
                std::vector<MemoryAccess> accesses;   // Recent memory accesses
                std::unordered_map<uint32_t, size_t>
                    poolUsage;                           // Usage by pool
                std::vector<TimeSeriesData> timeSeries;  // Time series data
                std::vector<std::vector<float>>
                    heatmap;          // Memory access heatmap
                uint64_t lastUpdate;  // Last update timestamp
            };

            static MemoryVisualizer &Get();

            void Initialize(const Config &config);
            void Shutdown();

            // Visualization control
            void StartVisualization();
            void StopVisualization();
            void PauseVisualization();
            void ResumeVisualization();
            bool IsVisualizing() const;

            // Data collection
            void OnMemoryAllocation(void *ptr,
                                    size_t size,
                                    uint32_t poolIndex,
                                    const char *tag = nullptr);
            void OnMemoryDeallocation(void *ptr, uint32_t poolIndex);
            void OnMemoryAccess(void *ptr, size_t size, bool isWrite);
            void OnPoolCreated(uint32_t poolIndex, size_t blockSize);
            void OnPoolDestroyed(uint32_t poolIndex);

            // Data export
            void ExportToFile(const std::string &filename) const;
            void GenerateReport(const std::string &filename) const;
            void DumpStats() const;

            // Data visualization
            void GenerateMemoryMap(const std::string &filename) const;
            void GenerateHeatmap(const std::string &filename) const;
            void GenerateTimeSeries(const std::string &filename) const;
            float CalculateFragmentation() const;

            // Visualization callbacks
            void SetUpdateCallback(
                std::function<void(const VisualizationData &)> callback);
            const VisualizationData &GetCurrentData() const;

          private:
            MemoryVisualizer() = default;
            ~MemoryVisualizer() = default;
            MemoryVisualizer(const MemoryVisualizer &) = delete;
            MemoryVisualizer &operator=(const MemoryVisualizer &) = delete;

            void UpdateVisualization();
            void TakeSnapshot();
            void ProcessMemoryAccess(const MemoryAccess &access);
            void CleanupOldData();
            void NotifyUpdateCallbacks();

            // Data export helpers
            void ExportToJSON(const std::string &filename) const;
            void ExportToCSV(const std::string &filename) const;

            // Memory management helpers
            void UpdateHeatmap(void *ptr, size_t size, bool isWrite);
            void AddTimeSeriesPoint();
            void MergeAdjacentRegions();
            void CompressTimeSeriesData();

            Config config;
            VisualizationData data;
            std::unordered_map<void *, MemoryBlock> activeBlocks;
            std::function<void(const VisualizationData &)> updateCallback;

            bool isRunning{false};
            bool isPaused{false};
            bool shouldStop{false};
            std::thread updateThread;
            mutable std::mutex mutex;
            std::condition_variable condition;
        };

    }  // namespace Memory
}  // namespace Engine
