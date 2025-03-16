
#pragma once

#include <array>
#include <vector>

#include "Core/Memory/MemorySystem.h"

namespace Engine {

    /**
     * @brief Standard heap allocator using system malloc/free
     */
    class HeapAllocator : public IAllocator {
      public:
        HeapAllocator();
        ~HeapAllocator() override;

        void *Allocate(size_t size,
                       size_t alignment = alignof(std::max_align_t)) override;
        void Deallocate(void *ptr) override;
        size_t GetAllocationSize(void *ptr) const override;
        bool OwnsAllocation(void *ptr) const override;
        const MemoryStats &GetStats() const override;
        void Reset() override;

      private:
        MemoryStats m_stats;
        mutable std::mutex m_mutex;
        std::unordered_map<void *, size_t> m_allocations;
    };

    /**
     * @brief Pool allocator for fixed-size allocations
     */
    class PoolAllocator : public IAllocator {
      public:
        PoolAllocator(size_t blockSize, size_t blockCount);
        ~PoolAllocator() override;

        void *Allocate(size_t size,
                       size_t alignment = alignof(std::max_align_t)) override;
        void Deallocate(void *ptr) override;
        size_t GetAllocationSize(void *ptr) const override;
        bool OwnsAllocation(void *ptr) const override;
        const MemoryStats &GetStats() const override;
        void Reset() override;

      private:
        struct Block {
            Block *next;
        };

        void Initialize();

        size_t m_blockSize;
        size_t m_blockCount;
        std::vector<uint8_t> m_memory;
        Block *m_freeList;
        MemoryStats m_stats;
        mutable std::mutex m_mutex;
    };

    /**
     * @brief Stack allocator for temporary allocations
     */
    class StackAllocator : public IAllocator {
      public:
        StackAllocator(size_t size);
        ~StackAllocator() override;

        void *Allocate(size_t size,
                       size_t alignment = alignof(std::max_align_t)) override;
        void Deallocate(void *ptr) override;
        size_t GetAllocationSize(void *ptr) const override;
        bool OwnsAllocation(void *ptr) const override;
        const MemoryStats &GetStats() const override;
        void Reset() override;

        // Stack-specific operations
        size_t GetMarker() const;
        void FreeToMarker(size_t marker);

      private:
        std::vector<uint8_t> m_memory;
        size_t m_currentPos;
        MemoryStats m_stats;
        mutable std::mutex m_mutex;
    };

    /**
     * @brief Free list allocator for variable-size allocations
     */
    class FreeListAllocator : public IAllocator {
      public:
        FreeListAllocator(size_t size);
        ~FreeListAllocator() override;

        void *Allocate(size_t size,
                       size_t alignment = alignof(std::max_align_t)) override;
        void Deallocate(void *ptr) override;
        size_t GetAllocationSize(void *ptr) const override;
        bool OwnsAllocation(void *ptr) const override;
        const MemoryStats &GetStats() const override;
        void Reset() override;

      private:
        struct FreeBlock {
            size_t size;
            FreeBlock *next;
        };

        FreeBlock *FindFirstFit(size_t size, size_t alignment);
        void Coalesce();

        std::vector<uint8_t> m_memory;
        FreeBlock *m_freeList;
        MemoryStats m_stats;
        mutable std::mutex m_mutex;
    };

    /**
     * @brief Buddy allocator for power-of-two allocations
     */
    class BuddyAllocator : public IAllocator {
      public:
        BuddyAllocator(size_t size);
        ~BuddyAllocator() override;

        void *Allocate(size_t size,
                       size_t alignment = alignof(std::max_align_t)) override;
        void Deallocate(void *ptr) override;
        size_t GetAllocationSize(void *ptr) const override;
        bool OwnsAllocation(void *ptr) const override;
        const MemoryStats &GetStats() const override;
        void Reset() override;

      private:
        struct BuddyBlock {
            size_t size;
            bool allocated;
            BuddyBlock *buddy;
        };

        size_t GetBlockSize(size_t size) const;
        BuddyBlock *FindBuddy(BuddyBlock *block);
        void Split(BuddyBlock *block, size_t size);
        void Merge(BuddyBlock *block);

        std::vector<uint8_t> m_memory;
        BuddyBlock *m_root;
        MemoryStats m_stats;
        mutable std::mutex m_mutex;
    };

    /**
     * @brief Null allocator that always returns nullptr (for testing)
     */
    class NullAllocator : public IAllocator {
      public:
        void *Allocate(size_t size,
                       size_t alignment = alignof(std::max_align_t)) override;
        void Deallocate(void *ptr) override;
        size_t GetAllocationSize(void *ptr) const override;
        bool OwnsAllocation(void *ptr) const override;
        const MemoryStats &GetStats() const override;
        void Reset() override;

      private:
        MemoryStats m_stats;
    };

    /**
     * @brief Proxy allocator that forwards to another allocator with additional
     * tracking
     */
    class ProxyAllocator : public IAllocator {
      public:
        ProxyAllocator(IAllocator *baseAllocator);

        void *Allocate(size_t size,
                       size_t alignment = alignof(std::max_align_t)) override;
        void Deallocate(void *ptr) override;
        size_t GetAllocationSize(void *ptr) const override;
        bool OwnsAllocation(void *ptr) const override;
        const MemoryStats &GetStats() const override;
        void Reset() override;

      private:
        IAllocator *m_baseAllocator;
        MemoryStats m_stats;
        mutable std::mutex m_mutex;
    };

}  // namespace Engine
