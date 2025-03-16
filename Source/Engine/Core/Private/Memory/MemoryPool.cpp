
#include "Core/Memory/MemoryPool.h"

#include <algorithm>
#include <cstring>

#include "Core/Assert.h"
#include "Log/LogSystem.h"

namespace Engine {
    namespace Memory {

        MemoryPool::MemoryPool(const Config &tempconfig) : config(tempconfig) {
            ASSERT(config.blockSize > 0, "Block size must be greater than 0");
            ASSERT(config.initialBlockCount > 0,
                   "Initial block count must be greater than 0");
            ASSERT(config.alignment > 0 && IsPowerOfTwo(config.alignment),
                   "Alignment must be a power of 2");
            ASSERT(config.growthFactor >= 1.0f, "Growth factor must be >= 1.0");

            // Adjust block size to include alignment requirements
            config.blockSize = AlignUp(config.blockSize, config.alignment);

            if (!InitializePool()) {
                LOG_ERROR("Failed to initialize memory pool");
                return;
            }

            initialized = true;
        }

        MemoryPool::~MemoryPool() {
            // Free all allocated chunks
            for (auto &chunk : chunks) {
                DestroyChunk(chunk);
            }
            chunks.clear();
            freeList = nullptr;
        }

        bool MemoryPool::InitializePool() {
            // Create initial blocks
            Block *newBlocks = CreateBlocks(config.initialBlockCount);
            if (!newBlocks) {
                return false;
            }

            // Add chunk to the list
            Chunk chunk;
            chunk.memory = newBlocks;
            chunk.blockCount = config.initialBlockCount;
            chunks.push_back(chunk);

            // Initialize free list
            freeList = newBlocks;

            // Update stats
            stats.totalSize = config.blockSize * config.initialBlockCount;
            stats.blockCount = config.initialBlockCount;

            return true;
        }

        MemoryPool::Block *MemoryPool::CreateBlocks(size_t count) {
            // Calculate total size needed for each block
            size_t blockSize = sizeof(Block) + config.blockSize - 1;
            size_t totalSize = count * blockSize;

            // Allocate memory
            void *memory = std::malloc(totalSize);
            if (!memory) {
                return nullptr;
            }

            // Initialize blocks
            Block *firstBlock = static_cast<Block *>(memory);
            Block *currentBlock = firstBlock;

            for (size_t i = 0; i < count - 1; ++i) {
                // Calculate next block address
                Block *nextBlock = reinterpret_cast<Block *>(
                    reinterpret_cast<char *>(currentBlock) + blockSize);
                currentBlock->next = nextBlock;
                currentBlock = nextBlock;
            }

            // Last block points to nullptr
            currentBlock->next = nullptr;

            return firstBlock;
        }

        void MemoryPool::DestroyChunk(Chunk &chunk) {
            if (chunk.memory) {
                std::free(chunk.memory);
                chunk.memory = nullptr;
                chunk.blockCount = 0;
            }
        }

        void *MemoryPool::Allocate() {
            if (!initialized) {
                return nullptr;
            }

            // Check if we need to expand
            if (!freeList && !Expand()) {
                return nullptr;
            }

            // Get block from free list
            Block *block = freeList;
            freeList = block->next;

            // Update stats
            stats.usedSize += config.blockSize;
            stats.usedBlocks++;
            stats.utilization =
                static_cast<float>(stats.usedBlocks) / stats.blockCount;

            // Return aligned data pointer
            return AlignPointer(block->data, config.alignment);
        }

        void MemoryPool::Deallocate(void *ptr) {
            if (!ptr || !initialized) {
                return;
            }

            // Verify the pointer belongs to this pool
            if (!Owns(ptr)) {
                LOG_ERROR(
                    "Attempting to deallocate memory not owned by this pool");
                return;
            }

            // Convert pointer back to block
            Block *block = reinterpret_cast<Block *>(
                reinterpret_cast<char *>(ptr) - offsetof(Block, data));

            // Add to free list
            block->next = freeList;
            freeList = block;

            // Update stats
            stats.usedSize -= config.blockSize;
            stats.usedBlocks--;
            stats.utilization =
                static_cast<float>(stats.usedBlocks) / stats.blockCount;

            // Check if we should shrink the pool
            if (stats.utilization < 0.25f) {
                Shrink();
            }
        }

        bool MemoryPool::Expand() {
            if (!config.growOnDemand ||
                (config.maxBlockCount > 0 &&
                 stats.blockCount >= config.maxBlockCount)) {
                return false;
            }

            // Calculate new block count
            size_t newBlockCount =
                static_cast<size_t>(stats.blockCount * config.growthFactor);
            if (config.maxBlockCount > 0) {
                newBlockCount = std::min(
                    newBlockCount, config.maxBlockCount - stats.blockCount);
            }

            // Create new blocks
            Block *newBlocks = CreateBlocks(newBlockCount);
            if (!newBlocks) {
                return false;
            }

            // Add new chunk
            Chunk chunk;
            chunk.memory = newBlocks;
            chunk.blockCount = newBlockCount;
            chunks.push_back(chunk);

            // Add to free list
            Block *lastBlock = newBlocks;
            while (lastBlock->next) {
                lastBlock = lastBlock->next;
            }
            lastBlock->next = freeList;
            freeList = newBlocks;

            // Update stats
            stats.totalSize += config.blockSize * newBlockCount;
            stats.blockCount += newBlockCount;
            stats.expansionCount++;
            stats.utilization =
                static_cast<float>(stats.usedBlocks) / stats.blockCount;

            return true;
        }

        void MemoryPool::Shrink() {
            if (chunks.size() <= 1) {
                return;  // Keep at least one chunk
            }

            // Calculate how many blocks we can free
            size_t targetBlocks = static_cast<size_t>(
                stats.usedBlocks * 1.5f);  // Keep 50% headroom

            while (chunks.size() > 1 && stats.blockCount > targetBlocks) {
                // Remove last chunk
                Chunk &chunk = chunks.back();
                stats.blockCount -= chunk.blockCount;
                stats.totalSize -= config.blockSize * chunk.blockCount;

                // Update free list
                Block *prev = nullptr;
                Block *current = freeList;

                while (current) {
                    if (IsAddressInChunk(current, chunk)) {
                        if (prev) {
                            prev->next = current->next;
                        } else {
                            freeList = current->next;
                        }
                    } else {
                        prev = current;
                    }
                    current = current->next;
                }

                DestroyChunk(chunk);
                chunks.pop_back();
            }

            stats.utilization =
                static_cast<float>(stats.usedBlocks) / stats.blockCount;
        }

        bool MemoryPool::Owns(void *ptr) const {
            if (!ptr || !initialized) {
                return false;
            }

            // Check if pointer is within any chunk
            for (const auto &chunk : chunks) {
                if (IsAddressInChunk(ptr, chunk)) {
                    return true;
                }
            }

            return false;
        }

        bool MemoryPool::IsAddressInChunk(void *ptr, const Chunk &chunk) const {
            char *start = static_cast<char *>(chunk.memory);
            char *end = start + (chunk.blockCount *
                                 (sizeof(Block) + config.blockSize - 1));
            char *addr = static_cast<char *>(ptr);

            return addr >= start && addr < end;
        }

        void MemoryPool::Reset() {
            if (!initialized) {
                return;
            }

            // Keep only the first chunk
            while (chunks.size() > 1) {
                DestroyChunk(chunks.back());
                chunks.pop_back();
            }

            // Reset free list to point to all blocks in the first chunk
            if (!chunks.empty()) {
                freeList = static_cast<Block *>(chunks[0].memory);
                Block *current = freeList;

                for (size_t i = 0; i < chunks[0].blockCount - 1; ++i) {
                    current->next = reinterpret_cast<Block *>(
                        reinterpret_cast<char *>(current) + sizeof(Block) +
                        config.blockSize - 1);
                    current = current->next;
                }
                current->next = nullptr;
            }

            // Reset stats
            stats = Stats{};
            stats.totalSize = config.blockSize * config.initialBlockCount;
            stats.blockCount = config.initialBlockCount;
        }

        bool MemoryPool::CanAllocate() const {
            return initialized && (freeList != nullptr ||
                                   (config.growOnDemand &&
                                    (config.maxBlockCount == 0 ||
                                     stats.blockCount < config.maxBlockCount)));
        }

        void MemoryPool::ValidatePool() const {
            if (!initialized) {
                return;
            }

            // Count blocks in free list
            size_t freeCount = 0;
            Block *current = freeList;

            while (current) {
                // Verify block belongs to a chunk
                bool found = false;
                for (const auto &chunk : chunks) {
                    if (IsAddressInChunk(current, chunk)) {
                        found = true;
                        break;
                    }
                }

                ASSERT(found, "Free block not found in any chunk");

                freeCount++;
                current = current->next;
            }

            // Verify block counts
            ASSERT(stats.blockCount == stats.usedBlocks + freeCount,
                   "Block count mismatch");

            // Verify utilization
            float expectedUtilization =
                static_cast<float>(stats.usedBlocks) / stats.blockCount;
            ASSERT(std::abs(stats.utilization - expectedUtilization) < 0.001f,
                   "Utilization calculation error");
        }

        void MemoryPool::DumpStats() const {
            LOG_INFO("=== Memory Pool Stats ===");
            LOG_INFO("Block size: {} bytes", config.blockSize);
            LOG_INFO("Total blocks: {}", stats.blockCount);
            LOG_INFO("Used blocks: {}", stats.usedBlocks);
            LOG_INFO("Total size: {} bytes", stats.totalSize);
            LOG_INFO("Used size: {} bytes", stats.usedSize);
            LOG_INFO("Utilization: {:.2f}%", stats.utilization * 100.0f);
            LOG_INFO("Expansion count: {}", stats.expansionCount);
            LOG_INFO("Chunk count: {}", chunks.size());
        }

        bool MemoryPool::IsPowerOfTwo(size_t value) {
            return value > 0 && (value & (value - 1)) == 0;
        }

        void *MemoryPool::AlignPointer(void *ptr, size_t alignment) {
            return reinterpret_cast<void *>(
                (reinterpret_cast<uintptr_t>(ptr) + (alignment - 1)) &
                ~(alignment - 1));
        }

        size_t MemoryPool::AlignUp(size_t size, size_t alignment) {
            return (size + alignment - 1) & ~(alignment - 1);
        }

    }  // namespace Memory
}  // namespace Engine
