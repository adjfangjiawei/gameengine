
#pragma once

#include <mutex>
#include <vector>

#include "Core/Memory/IAllocator.h"

namespace Engine {
    namespace Memory {

        class PoolAllocator : public IAllocator {
          public:
            struct Config {
                size_t blockSize;        // Size of each block
                size_t initialChunks;    // Initial number of chunks
                size_t expansionChunks;  // Number of chunks to add when pool is
                                         // full
                size_t maxChunks;        // Maximum number of chunks
                size_t alignment;        // Memory alignment requirement
                bool allowExpansion;     // Whether to allow pool expansion

                Config()
                    : blockSize(0),
                      initialChunks(0),
                      expansionChunks(0),
                      maxChunks(0),
                      alignment(DEFAULT_ALIGNMENT),
                      allowExpansion(true) {}
            };

            explicit PoolAllocator(const Config &config);
            ~PoolAllocator() override;

            // Disable copy operations
            PoolAllocator(const PoolAllocator &) = delete;
            PoolAllocator &operator=(const PoolAllocator &) = delete;

            // Enable move operations
            PoolAllocator(PoolAllocator &&other) noexcept;
            PoolAllocator &operator=(PoolAllocator &&other) noexcept;

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
            bool OwnsAllocation(void *ptr) const override;
            const MemoryStats &GetStats() const override;

            // Pool-specific operations
            bool Expand(size_t chunks) override;
            void Shrink() override;
            void Reset() override;

          private:
            struct Chunk {
                void *memory;
                size_t size;
                size_t used;
                void *next;

                Chunk() : memory(nullptr), size(0), used(0), next(nullptr) {}
            };

            struct BlockHeader {
                Chunk *chunk;
                size_t size;
                size_t adjustment;
            };

            Config config;
            std::vector<Chunk> chunks;
            void *freeList;
            MemoryStats stats;
            mutable std::mutex mutex;

            bool InitializeChunk(Chunk &chunk);
            bool IsAddressInChunk(const Chunk &chunk, void *ptr) const;
            void *AlignBlock(void *ptr, size_t alignment, size_t &adjustment);
            BlockHeader *GetBlockHeader(void *ptr) const;
        };

    }  // namespace Memory
}  // namespace Engine
