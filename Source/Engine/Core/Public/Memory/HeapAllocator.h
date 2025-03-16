
#pragma once

#include <chrono>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>

#include "IAllocator.h"

namespace Engine {
    namespace Memory {

        class HeapAllocator : public IAllocator {
          public:
            struct Config {
                bool trackAllocations{
                    true};  // Whether to track allocation details
                bool enableDebugInfo{
                    true};  // Whether to store debug information
                bool enableGuardBytes{true};  // Whether to add memory guards
                size_t debugBackTraceDepth{
                    32};              // Depth of stack trace for debug info
                size_t guardSize{8};  // Size of guard regions in bytes
                bool enableDefragmentation{
                    true};  // Whether to enable automatic defragmentation
                size_t defragmentationThreshold{
                    75};  // Fragmentation percentage to trigger defragmentation
            };

            struct DetailedMemoryStats : public MemoryStats {
                std::unordered_map<std::string, size_t> allocationsByType;
                std::unordered_map<std::string, size_t> allocationsByFile;
                size_t fragmentedMemory{0};
                size_t headerOverhead{0};
                size_t guardOverhead{0};
                double fragmentationRatio{0.0};

                void Clear() {
                    MemoryStats::operator=({});
                    allocationsByType.clear();
                    allocationsByFile.clear();
                    fragmentedMemory = 0;
                    headerOverhead = 0;
                    guardOverhead = 0;
                    fragmentationRatio = 0.0;
                }
            };

            explicit HeapAllocator(const Config &config);
            ~HeapAllocator() override;

            // IAllocator interface implementation
            void *Allocate(
                size_t size,
                size_t alignment = DEFAULT_ALIGNMENT,
                AllocFlags flags = AllocFlags::None,
                const MemoryLocation &location = MEMORY_LOCATION) override;

            void Deallocate(
                void *ptr,
                const MemoryLocation &location = MEMORY_LOCATION) override;

            void *AllocateTracked(
                size_t size,
                size_t alignment,
                const char *typeName,
                AllocFlags flags = AllocFlags::None,
                const MemoryLocation &location = MEMORY_LOCATION) override;

            size_t GetAllocationSize(void *ptr) const override;
            bool OwnsAllocation(void *ptr)
                const override;  // Renamed from IsOwner for consistency
            const MemoryStats &GetStats() const override;
            void Reset() override;

            // Debug functions
            void DumpAllocationInfo() const override;
            void ValidateHeap() const override;
            void CheckForLeaks() const;
            void AnalyzeFragmentation() const;
            void DumpMemoryStats() const;

            // Enhanced memory management
            void Defragment();
            const DetailedMemoryStats &GetDetailedStats() const;
            void SetDefragmentationThreshold(size_t percentage);
            double GetFragmentationRatio() const;

          private:
            static constexpr std::uint8_t GUARD_PATTERN = 0xFD;
            static constexpr std::uint8_t FREED_PATTERN = 0xDD;

            struct AllocationHeader {
                size_t size;               // Size of the allocation
                size_t alignment;          // Alignment requirement
                const char *typeName;      // Type name for tracked allocations
                MemoryLocation location;   // Source location of allocation
                uint64_t timestamp;        // Allocation timestamp
                std::thread::id threadId;  // Thread that made the allocation
                void *backTrace[32];       // Stack trace (if debug enabled)
                size_t backTraceSize;      // Number of stack trace entries
                bool isFreed;              // Flag to detect double frees
                uint32_t allocCount;       // Unique allocation counter
                uint32_t checksum;  // Header checksum for corruption detection
                size_t paddingFront;  // Padding size at front
                size_t paddingBack;   // Padding size at back
            };

            struct AllocationInfo {
                AllocationHeader header;
                size_t actualSize;   // Total size including header, guards and
                                     // padding
                void *originalPtr;   // Original allocated pointer
                size_t paddingSize;  // Size of padding used for alignment
                std::chrono::steady_clock::time_point allocationTime;
                bool needsDefragmentation;  // Flag indicating if this block
                                            // needs defragmentation
            };

            struct FragmentationInfo {
                size_t totalFragments;
                size_t averageFragmentSize;
                size_t largestFragment;
                size_t smallestFragment;
                double fragmentationRatio;
                std::vector<std::pair<size_t, size_t>>
                    fragmentSizes;  // size, count pairs
            };

            void *AllocateInternal(size_t size,
                                   size_t alignment,
                                   const char *typeName,
                                   AllocFlags flags,
                                   const MemoryLocation &location);
            void CaptureBackTrace(AllocationHeader &header);
            void ValidateGuardBytes(const AllocationInfo &info) const;
            void SetGuardBytes(void *ptr, size_t size);
            uint32_t CalculateHeaderChecksum(
                const AllocationHeader &header) const;
            void ValidateHeader(const AllocationHeader &header) const;
            FragmentationInfo AnalyzeFragmentationInternal() const;
            size_t CalculatePaddingSize(void *ptr, size_t alignment) const;
            void FillFreedMemory(void *ptr, size_t size);
            void UpdateDetailedStats();
            bool TryDefragment(AllocationInfo &info);
            void CheckAndTriggerDefragmentation();

            Config config;
            std::unordered_map<void *, AllocationInfo> allocations;
            DetailedMemoryStats detailedStats;
            mutable std::shared_mutex mutex;
            uint32_t nextAllocCount{0};
            size_t totalDefragmentationCount{0};
            std::chrono::steady_clock::time_point lastDefragmentationTime;
        };

    }  // namespace Memory
}  // namespace Engine
