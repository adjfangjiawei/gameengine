
#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

#include "Core/CoreTypes.h"

namespace Engine {
    namespace Memory {

        // Memory alignment constants
        constexpr size_t DEFAULT_ALIGNMENT = 8;
        constexpr size_t CACHE_LINE_SIZE = 64;
        constexpr size_t PAGE_SIZE = 4096;

        // Memory allocation flags
        enum class AllocFlags {
            None = 0,
            ZeroMemory = 1 << 0,
            Aligned = 1 << 1
        };

        inline AllocFlags operator|(AllocFlags a, AllocFlags b) {
            return static_cast<AllocFlags>(static_cast<int>(a) |
                                           static_cast<int>(b));
        }

        inline AllocFlags operator&(AllocFlags a, AllocFlags b) {
            return static_cast<AllocFlags>(static_cast<int>(a) &
                                           static_cast<int>(b));
        }

        // Memory location tracking
        struct MemoryLocation {
            std::string file;
            int line;
            std::string function;
        };

#define MEMORY_LOCATION \
    MemoryLocation { __FILE__, __LINE__, __FUNCTION__ }

        /**
         * @brief Memory allocation statistics
         */
        struct MemoryStats {
            size_t totalAllocated{0};     // Total allocated memory
            size_t totalDeallocated{0};   // Total deallocated memory
            size_t currentUsage{0};       // Current memory usage
            size_t peakUsage{0};          // Peak memory usage
            size_t allocationCount{0};    // Number of allocations
            size_t deallocationCount{0};  // Number of deallocations

            // Additional statistics for specific allocators
            size_t blockSize{
                0};  // Size of each block (for pool/buddy allocators)
            size_t totalBlocks{0};  // Total number of blocks
            size_t usedBlocks{0};   // Number of used blocks
            size_t wastedMemory{
                0};  // Wasted memory due to fragmentation or alignment

            // Advanced stats for debugging
            size_t headerOverhead{0};  // Memory used for allocation headers
            size_t guardOverhead{0};   // Memory used for guard bytes
            double fragmentationRatio{0.0};  // Memory fragmentation ratio

            // Reset all stats to defaults
            void Clear() { *this = MemoryStats{}; }
        };

        /**
         * @brief Detailed memory statistics with allocation tracking
         */
        struct DetailedMemoryStats : public MemoryStats {
            std::unordered_map<std::string, size_t> allocationsByType;
            std::unordered_map<std::string, size_t> allocationsByFile;
            size_t fragmentedMemory{0};

            void Clear() {
                MemoryStats::Clear();
                allocationsByType.clear();
                allocationsByFile.clear();
                fragmentedMemory = 0;
            }
        };

        /**
         * @brief Memory allocation tracking information
         */
        struct AllocationInfo {
            void *address;             // Memory address
            size_t size;               // Allocation size
            const char *file;          // Source file
            int line;                  // Source line
            const char *function;      // Function name
            const char *tag;           // Optional tag for grouping
            std::thread::id threadId;  // Thread that made the allocation
            uint64_t timestamp;        // Allocation timestamp
        };

        // Memory utilities
        inline void *AlignPointer(void *ptr, size_t alignment) {
            uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
            uintptr_t aligned = (addr + alignment - 1) & ~(alignment - 1);
            return reinterpret_cast<void *>(aligned);
        }

        inline bool IsAligned(void *ptr, size_t alignment) {
            return (reinterpret_cast<uintptr_t>(ptr) & (alignment - 1)) == 0;
        }

        inline bool IsPowerOfTwo(size_t x) { return x && !(x & (x - 1)); }

        inline size_t AlignUp(size_t size, size_t alignment) {
            return (size + alignment - 1) & ~(alignment - 1);
        }

    }  // namespace Memory
}  // namespace Engine

// For backward compatibility
namespace Engine {
    using MemoryStats = Engine::Memory::MemoryStats;
    using DetailedMemoryStats = Engine::Memory::DetailedMemoryStats;
    using AllocationInfo = Engine::Memory::AllocationInfo;
}  // namespace Engine
