
#pragma once

#include <mutex>
#include <stack>

#include "IAllocator.h"

namespace Engine {
    namespace Memory {

        class StackAllocator : public IAllocator {
          public:
            struct Config {
                size_t totalSize;   // Total size of the memory block
                size_t stackDepth;  // Maximum number of markers (for nested
                                    // allocations)
            };

            explicit StackAllocator(const Config &config);
            ~StackAllocator() override;

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

            // Stack-specific operations
            size_t GetMarker() const;          // Get current position marker
            void FreeToMarker(size_t marker);  // Free memory up to marker

          private:
            struct AllocationHeader {
                size_t size;        // Size of the allocation
                size_t adjustment;  // Alignment adjustment
                size_t prevMarker;  // Previous marker position
            };

            void *memory;                // Base memory pointer
            void *current;               // Current allocation position
            size_t totalSize;            // Total size of memory block
            std::stack<size_t> markers;  // Stack of allocation markers
            MemoryStats stats;           // Memory statistics
            mutable std::mutex mutex;    // Thread safety mutex
        };

    }  // namespace Memory
}  // namespace Engine
