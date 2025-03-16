
#pragma once

#include <mutex>

#include "IAllocator.h"

namespace Engine {
    namespace Memory {

        class LinearAllocator : public IAllocator {
          public:
            struct Config {
                size_t totalSize;  // Total size of the memory block
                bool allowReset;   // Whether to allow reset operations
            };

            explicit LinearAllocator(const Config &config);
            ~LinearAllocator() override;

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
            void Reset() override;

          private:
            struct AllocationHeader {
                size_t size;        // Size of the allocation
                size_t adjustment;  // Alignment adjustment
            };

            void *memory;              // Base memory pointer
            void *current;             // Current allocation position
            size_t totalSize;          // Total size of memory block
            bool allowReset;           // Whether reset is allowed
            MemoryStats stats;         // Memory statistics
            mutable std::mutex mutex;  // Thread safety mutex
        };

    }  // namespace Memory
}  // namespace Engine
