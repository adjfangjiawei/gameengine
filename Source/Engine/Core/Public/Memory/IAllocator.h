
#pragma once

#include "Memory/MemoryTypes.h"

namespace Engine {
    namespace Memory {

        /**
         * @brief Base interface for all memory allocators
         */
        class IAllocator {
          public:
            virtual ~IAllocator() = default;

            // Core allocation interface
            virtual void *Allocate(
                size_t size,
                size_t alignment = DEFAULT_ALIGNMENT,
                AllocFlags flags = AllocFlags::None,
                const MemoryLocation &location = MEMORY_LOCATION) = 0;

            virtual void Deallocate(
                void *ptr,
                const MemoryLocation &location = MEMORY_LOCATION) = 0;

            // Memory tracking
            virtual void *AllocateTracked(
                size_t size,
                size_t alignment,
                const char *typeName,
                AllocFlags flags = AllocFlags::None,
                const MemoryLocation &location = MEMORY_LOCATION) = 0;

            virtual size_t GetAllocationSize(void *ptr) const = 0;
            virtual bool OwnsAllocation(
                void *ptr) const = 0;  // Renamed from IsOwner for clarity

            // Memory stats
            virtual const MemoryStats &GetStats() const = 0;

            // Optional operations
            virtual void Reset() {}
            virtual bool Expand(size_t size) {
                (void)size;  // Avoid unused parameter warning
                return false;
            }
            virtual void Shrink() {}

            // Debug operations
            virtual void ValidateHeap() const {}
            virtual void DumpAllocationInfo() const {}
        };

    }  // namespace Memory
}  // namespace Engine

// For backward compatibility
namespace Engine {
    using IAllocator = Engine::Memory::IAllocator;
}
