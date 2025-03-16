
#pragma once

#include <condition_variable>
#include <cstring>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "Core/Memory/MemoryTypes.h"
#include "CoreTypes.h"

namespace Engine {
    namespace Memory {

        class MemoryDefragmenter {
          public:
            struct Config {
                size_t minFragmentSize{
                    1024};  // Minimum size to consider as fragment
                size_t targetUtilization{
                    90};  // Target memory utilization percentage
                size_t maxMoveSize{
                    1024 * 1024};  // Maximum size to move in one operation
                bool enableIncrementalDefrag{
                    true};  // Enable incremental defragmentation
                bool enableBackgroundDefrag{
                    true};  // Enable background defragmentation
                uint32_t defragInterval{
                    1000};  // Interval between defrag operations (ms)
                float fragmentationThreshold{
                    0.3f};  // Threshold to trigger defragmentation
            };

            struct FragmentInfo {
                void *address;  // Fragment address
                size_t size;    // Fragment size
                bool canMove;   // Whether the fragment can be moved
                float score;  // Fragmentation score (higher = more fragmented)
            };

            struct DefragStats {
                size_t totalMoves;      // Total number of block moves
                size_t totalMoved;      // Total bytes moved
                size_t fragmentsFound;  // Number of fragments found
                size_t fragmentsFixed;  // Number of fragments fixed
                size_t spaceRecovered;  // Total space recovered
                double timeSpent;       // Time spent defragmenting
                float improvement;      // Improvement in fragmentation
            };

            struct MoveOperation {
                void *source;       // Source address
                void *destination;  // Destination address
                size_t size;        // Size to move
                std::function<void()>
                    updateReferences;  // Function to update references
                float priority;        // Operation priority
            };

            static MemoryDefragmenter &Get();

            void Initialize(const Config &config);
            void Shutdown();

            // Defragmentation control
            void StartDefragmentation();
            void StopDefragmentation();
            void PauseDefragmentation();
            void ResumeDefragmentation();
            bool IsDefragmenting() const;

            // Manual defragmentation
            void DefragmentNow();
            void DefragmentRegion(void *start, size_t size);
            void DefragmentIncremental(size_t maxMoveSize);

            // Fragment analysis
            std::vector<FragmentInfo> AnalyzeFragmentation() const;
            float CalculateFragmentationScore() const;
            size_t EstimateRecoverableSpace() const;

            // Statistics and reporting
            const DefragStats &GetStats() const;
            void ResetStats();
            void GenerateReport(const char *filename) const;

            // Registration
            void RegisterMovableBlock(void *ptr,
                                      size_t size,
                                      std::function<void(void *)> moveCallback);
            void UnregisterBlock(void *ptr);

          private:
            MemoryDefragmenter() = default;
            ~MemoryDefragmenter() = default;
            MemoryDefragmenter(const MemoryDefragmenter &) = delete;
            MemoryDefragmenter &operator=(const MemoryDefragmenter &) = delete;

            struct Block {
                void *address;
                size_t size;
                bool isMovable;
                std::function<void(void *)> moveCallback;
            };

            struct DefragmentationTask {
                void *region;
                size_t size;
                float priority;
                bool operator<(const DefragmentationTask &other) const {
                    return priority < other.priority;
                }
            };

            // Internal helpers
            void ProcessDefragmentationQueue();
            bool TryDefragmentBlock(const Block &block);
            void MoveBlock(void *source, void *destination, size_t size);
            void UpdateBlockReferences(void *oldAddress, void *newAddress);
            bool IsSpaceAvailable(void *address, size_t size) const;
            void *FindBestFitSpace(size_t size) const;
            float CalculateBlockPriority(const Block &block) const;
            void LogDefragmentation(const Block &block, void *newAddress) const;

            // Background processing
            void StartBackgroundThread();
            void StopBackgroundThread();
            void BackgroundThreadFunction();
            void ProcessIncrementalDefrag();

            Config config;
            DefragStats stats;
            std::vector<Block> blocks;
            std::priority_queue<DefragmentationTask> defragQueue;
            std::vector<MoveOperation> pendingMoves;

            bool isRunning{false};
            bool isPaused{false};
            bool shouldStop{false};
            std::thread backgroundThread;
            mutable std::mutex mutex;
            std::condition_variable condition;
        };

    }  // namespace Memory
}  // namespace Engine
