
#pragma once

#include <mutex>
#include <vector>

#include "Core/CoreTypes.h"
#include "Core/Memory/MemoryTypes.h"

namespace Engine {
    namespace Memory {

        class MemoryPool {
          public:
            struct Config {
                size_t blockSize;          // Size of each memory block
                size_t initialBlockCount;  // Initial number of blocks
                size_t maxBlockCount;      // Maximum number of blocks (0 for
                                           // unlimited)
                size_t alignment;          // Memory alignment requirement
                bool growOnDemand;         // Whether to allow pool expansion
                float growthFactor;  // Growth factor when expanding (e.g., 1.5)
            };

            explicit MemoryPool(const Config &config);
            ~MemoryPool();

            // Allocation/deallocation
            void *Allocate();
            void Deallocate(void *ptr);

            // Pool management
            bool Expand();             // Expand pool when needed
            void Shrink();             // Release unused memory chunks
            void Reset();              // Reset pool to initial state
            bool CanAllocate() const;  // Check if allocation is possible
            bool Owns(
                void *ptr) const;  // Check if pointer belongs to this pool

            // Debug and validation
            void ValidatePool() const;  // Validate pool integrity
            void DumpStats() const;     // Output pool statistics

          private:
            struct Block {
                Block *next;   // Next block in free list
                char data[1];  // Start of block data (flexible array member)
            };

            struct Chunk {
                void *memory;       // Chunk memory
                size_t blockCount;  // Number of blocks in chunk
            };

            struct Stats {
                size_t blockCount = 0;      // Total number of blocks
                size_t usedBlocks = 0;      // Number of allocated blocks
                size_t totalSize = 0;       // Total memory allocated
                size_t usedSize = 0;        // Memory currently in use
                size_t expansionCount = 0;  // Number of expansions
                float utilization = 0.0f;   // Current utilization ratio
            };

            // Initialization
            bool InitializePool();
            Block *CreateBlocks(size_t count);
            void DestroyChunk(Chunk &chunk);

            // Helper functions
            bool IsAddressInChunk(void *ptr, const Chunk &chunk) const;
            static bool IsPowerOfTwo(size_t value);
            static void *AlignPointer(void *ptr, size_t alignment);
            static size_t AlignUp(size_t size, size_t alignment);

            // Member variables
            Config config;
            std::vector<Chunk> chunks;
            Block *freeList = nullptr;
            Stats stats;
            bool initialized = false;
        };

        // Helper for creating common pool configurations
        struct MemoryPoolConfig {
            static MemoryPool::Config Create(
                size_t blockSize,
                size_t initialBlocks = 32,
                size_t alignment = DEFAULT_ALIGNMENT) {
                MemoryPool::Config config;
                config.blockSize = blockSize;
                config.initialBlockCount = initialBlocks;
                config.maxBlockCount = 0;  // Unlimited
                config.alignment = alignment;
                config.growOnDemand = true;
                config.growthFactor = 1.5f;
                return config;
            }

            static MemoryPool::Config CreateFixed(
                size_t blockSize,
                size_t totalBlocks,
                size_t alignment = DEFAULT_ALIGNMENT) {
                MemoryPool::Config config;
                config.blockSize = blockSize;
                config.initialBlockCount = totalBlocks;
                config.maxBlockCount = totalBlocks;
                config.alignment = alignment;
                config.growOnDemand = false;
                config.growthFactor = 1.0f;
                return config;
            }
        };

    }  // namespace Memory
}  // namespace Engine
