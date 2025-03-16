
#include "Memory/PoolAllocator.h"

#include <cstring>

#include "Assert.h"
#include "Log/LogSystem.h"

namespace Engine {
    namespace Memory {

        // Move constructor
        PoolAllocator::PoolAllocator(PoolAllocator &&other) noexcept
            : config(std::move(other.config)),
              chunks(std::move(other.chunks)),
              freeList(other.freeList),
              stats(std::move(other.stats)) {
            // Reset the moved-from object to a valid but empty state
            other.freeList = nullptr;
        }

        // Move assignment operator
        PoolAllocator &PoolAllocator::operator=(
            PoolAllocator &&other) noexcept {
            if (this != &other) {
                // Clean up this object's resources first
                for (auto &chunk : chunks) {
                    std::free(chunk.memory);
                }
                chunks.clear();

                // Move resources from other
                config = std::move(other.config);
                chunks = std::move(other.chunks);
                freeList = other.freeList;
                stats = std::move(other.stats);

                // Reset the moved-from object
                other.freeList = nullptr;
            }
            return *this;
        }

        PoolAllocator::PoolAllocator(const Config &config) : config(config) {
            ASSERT_MSG(config.blockSize > 0,
                       "Block size must be greater than 0",
                       AssertType::Fatal);
            ASSERT_MSG(config.initialChunks > 0,
                       "Initial chunks must be greater than 0",
                       AssertType::Fatal);
            ASSERT_MSG(IsPowerOfTwo(config.alignment),
                       "Alignment must be power of 2",
                       AssertType::Fatal);

            // Initialize first chunk
            Chunk chunk;
            if (InitializeChunk(chunk)) {
                chunks.push_back(chunk);

                // Update stats
                stats.blockSize = config.blockSize;
                stats.totalBlocks = config.initialChunks;
                stats.usedBlocks = 0;
            }
        }

        PoolAllocator::~PoolAllocator() {
            // Free all chunks
            for (auto &chunk : chunks) {
                std::free(chunk.memory);
            }
            chunks.clear();
            freeList = nullptr;
        }

        void *PoolAllocator::Allocate(
            size_t size,
            size_t alignment,
            AllocFlags flags,
            [[maybe_unused]] const MemoryLocation &location) {
            if (size > config.blockSize) {
                LOG_ERROR("Requested size {} exceeds block size {}",
                          size,
                          config.blockSize);
                return nullptr;
            }

            std::lock_guard<std::mutex> lock(mutex);

            // Try to get a block from the free list
            if (freeList) {
                size_t adjustment;
                void *alignedBlock =
                    AlignBlock(freeList, alignment, adjustment);
                if (alignedBlock) {
                    BlockHeader *header = reinterpret_cast<BlockHeader *>(
                        static_cast<char *>(alignedBlock) -
                        sizeof(BlockHeader));
                    header->size = size;
                    header->adjustment = adjustment;

                    freeList = *static_cast<void **>(freeList);

                    // Update stats
                    stats.usedBlocks++;
                    stats.allocationCount++;

                    // Zero memory if requested
                    if ((flags & AllocFlags::ZeroMemory) ==
                        AllocFlags::ZeroMemory) {
                        std::memset(alignedBlock, 0, size);
                    }

                    return alignedBlock;
                }
            }

            // Try to find a chunk with free blocks
            for (auto &chunk : chunks) {
                if (chunk.used < chunk.size) {
                    void *block = static_cast<char *>(chunk.memory) +
                                  (chunk.used * config.blockSize);
                    size_t adjustment;
                    void *alignedBlock =
                        AlignBlock(block, alignment, adjustment);
                    if (alignedBlock) {
                        BlockHeader *header = reinterpret_cast<BlockHeader *>(
                            static_cast<char *>(alignedBlock) -
                            sizeof(BlockHeader));
                        header->chunk = &chunk;
                        header->size = size;
                        header->adjustment = adjustment;

                        chunk.used++;

                        // Update stats
                        stats.usedBlocks++;
                        stats.allocationCount++;

                        // Zero memory if requested
                        if ((flags & AllocFlags::ZeroMemory) ==
                            AllocFlags::ZeroMemory) {
                            std::memset(alignedBlock, 0, size);
                        }

                        return alignedBlock;
                    }
                }
            }

            // If we can expand, create a new chunk
            if (config.allowExpansion &&
                (config.maxChunks == 0 || chunks.size() < config.maxChunks)) {
                chunks.emplace_back();
                Chunk &chunk = chunks.back();
                if (InitializeChunk(chunk)) {
                    void *block = chunk.memory;
                    size_t adjustment;
                    void *alignedBlock =
                        AlignBlock(block, alignment, adjustment);
                    if (alignedBlock) {
                        BlockHeader *header = reinterpret_cast<BlockHeader *>(
                            static_cast<char *>(alignedBlock) -
                            sizeof(BlockHeader));
                        header->chunk = &chunk;
                        header->size = size;
                        header->adjustment = adjustment;

                        chunk.used = 1;

                        // Update stats
                        stats.totalBlocks += config.initialChunks;
                        stats.usedBlocks++;
                        stats.allocationCount++;

                        // Zero memory if requested
                        if ((flags & AllocFlags::ZeroMemory) ==
                            AllocFlags::ZeroMemory) {
                            std::memset(alignedBlock, 0, size);
                        }

                        return alignedBlock;
                    }
                }
            }

            LOG_ERROR("Pool allocator out of memory");
            return nullptr;
        }

        void PoolAllocator::Deallocate(
            void *ptr, [[maybe_unused]] const MemoryLocation &location) {
            if (!ptr) return;

            std::lock_guard<std::mutex> lock(mutex);

            BlockHeader *header = GetBlockHeader(ptr);
            if (!header) {
                LOG_ERROR("Invalid pointer or corrupted header");
                return;
            }

            // Add to free list
            void *block = static_cast<char *>(ptr) - header->adjustment;
            *static_cast<void **>(block) = freeList;
            freeList = block;

            // Update stats
            stats.usedBlocks--;
            stats.deallocationCount++;
            if (header->chunk) {
                header->chunk->used--;
            }
        }

        void *PoolAllocator::AllocateTracked(size_t size,
                                             size_t alignment,
                                             const char *typeName,
                                             AllocFlags flags,
                                             const MemoryLocation &location) {
            void *ptr = Allocate(size, alignment, flags, location);
            if (ptr && typeName) {
                // Store type information if needed
            }
            return ptr;
        }

        size_t PoolAllocator::GetAllocationSize(void *ptr) const {
            if (!ptr) return 0;

            std::lock_guard<std::mutex> lock(mutex);
            BlockHeader *header = GetBlockHeader(ptr);
            return header ? header->size : 0;
        }

        bool PoolAllocator::OwnsAllocation(void *ptr) const {
            if (!ptr) return false;

            std::lock_guard<std::mutex> lock(mutex);
            for (const auto &chunk : chunks) {
                if (IsAddressInChunk(chunk, ptr)) {
                    return true;
                }
            }
            return false;
        }

        const MemoryStats &PoolAllocator::GetStats() const { return stats; }

        bool PoolAllocator::Expand(size_t chunks) {
            std::lock_guard<std::mutex> lock(mutex);

            if (!config.allowExpansion) {
                return false;
            }

            if (config.maxChunks > 0 &&
                this->chunks.size() + chunks > config.maxChunks) {
                return false;
            }

            size_t initialSize = this->chunks.size();
            for (size_t i = 0; i < chunks; ++i) {
                Chunk chunk;
                if (!InitializeChunk(chunk)) {
                    return false;
                }
                this->chunks.push_back(chunk);
            }

            stats.totalBlocks += chunks * config.initialChunks;
            return this->chunks.size() > initialSize;
        }

        void PoolAllocator::Shrink() {
            std::lock_guard<std::mutex> lock(mutex);

            // Keep at least one chunk
            if (chunks.size() <= 1) {
                return;
            }

            // Remove empty chunks from the end
            while (chunks.size() > 1 && chunks.back().used == 0) {
                std::free(chunks.back().memory);
                chunks.pop_back();
                stats.totalBlocks -= config.initialChunks;
            }
        }

        void PoolAllocator::Reset() {
            std::lock_guard<std::mutex> lock(mutex);

            // Keep only the first chunk
            while (chunks.size() > 1) {
                std::free(chunks.back().memory);
                chunks.pop_back();
            }

            // Reset the first chunk
            if (!chunks.empty()) {
                chunks[0].used = 0;
                chunks[0].next = nullptr;
            }

            // Reset free list
            freeList = nullptr;

            // Reset stats
            stats.usedBlocks = 0;
            stats.totalBlocks = config.initialChunks;
            stats.allocationCount = 0;
            stats.deallocationCount = 0;
        }

        bool PoolAllocator::InitializeChunk(Chunk &chunk) {
            // Allocate memory for the chunk
            chunk.memory = std::malloc(config.blockSize * config.initialChunks);
            if (!chunk.memory) {
                LOG_ERROR("Failed to allocate memory for pool chunk");
                return false;
            }

            // Initialize chunk properties
            chunk.size = config.initialChunks;
            chunk.used = 0;
            chunk.next = nullptr;

            // Initialize free list for this chunk
            void *current = chunk.memory;
            for (size_t i = 0; i < config.initialChunks - 1; ++i) {
                *static_cast<void **>(current) =
                    static_cast<char *>(current) + config.blockSize;
                current = static_cast<char *>(current) + config.blockSize;
            }
            *static_cast<void **>(current) = nullptr;

            return true;
        }

        bool PoolAllocator::IsAddressInChunk(const Chunk &chunk,
                                             void *ptr) const {
            return ptr >= chunk.memory &&
                   ptr < static_cast<char *>(chunk.memory) +
                             (chunk.size * config.blockSize);
        }

        void *PoolAllocator::AlignBlock(void *ptr,
                                        size_t alignment,
                                        size_t &adjustment) {
            uintptr_t address = reinterpret_cast<uintptr_t>(ptr);
            uintptr_t aligned =
                (address + sizeof(BlockHeader) + alignment - 1) &
                ~(alignment - 1);
            adjustment = aligned - address;
            return reinterpret_cast<void *>(aligned);
        }

        PoolAllocator::BlockHeader *PoolAllocator::GetBlockHeader(
            void *ptr) const {
            if (!ptr) return nullptr;
            return reinterpret_cast<BlockHeader *>(static_cast<char *>(ptr) -
                                                   sizeof(BlockHeader));
        }

    }  // namespace Memory
}  // namespace Engine
