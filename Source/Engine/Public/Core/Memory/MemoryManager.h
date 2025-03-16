
#pragma once

#include <mutex>
#include <string>
#include <unordered_map>

#include "IAllocator.h"

namespace Engine {
    namespace Memory {

        // Forward declarations of global memory functions
        void *Allocate(size_t size,
                       size_t alignment = DEFAULT_ALIGNMENT,
                       AllocFlags flags = AllocFlags::None,
                       const MemoryLocation &location = MEMORY_LOCATION);

        void Deallocate(void *ptr,
                        const MemoryLocation &location = MEMORY_LOCATION);

        // Memory Manager class
        class MemoryManager {
          public:
            static MemoryManager &Get();

            // Allocator management
            void RegisterAllocator(const char *name, IAllocator *allocator);
            void UnregisterAllocator(const char *name);
            IAllocator *GetAllocator(const char *name);
            IAllocator *GetDefaultAllocator();
            void SetDefaultAllocator(const char *name);

            // Memory tracking
            void EnableTracking(bool enable);
            bool IsTrackingEnabled() const;

            // Memory statistics
            MemoryStats GetTotalStats() const;
            MemoryStats GetAllocatorStats(const char *name) const;

            // Memory debugging
            void CheckLeaks();
            void DumpLeaks();
            void DumpStats();
            void ValidateHeap();

          private:
            MemoryManager() = default;
            MemoryManager(const MemoryManager &) = delete;
            MemoryManager &operator=(const MemoryManager &) = delete;

            std::unordered_map<std::string, IAllocator *> allocators;
            IAllocator *defaultAllocator{nullptr};
            bool trackingEnabled{false};
            mutable std::mutex mutex;
        };

        // Global memory functions implementation
        inline void *Allocate(size_t size,
                              size_t alignment,
                              AllocFlags flags,
                              const MemoryLocation &location) {
            return MemoryManager::Get().GetDefaultAllocator()->Allocate(
                size, alignment, flags, location);
        }

        inline void Deallocate(void *ptr, const MemoryLocation &location) {
            if (ptr) {
                MemoryManager::Get().GetDefaultAllocator()->Deallocate(
                    ptr, location);
            }
        }

        // Template functions for type-safe allocation
        template <typename T, typename... Args>
        T *New(Args &&...args) {
            void *memory = MemoryManager::Get().GetDefaultAllocator()->Allocate(
                sizeof(T), alignof(T), AllocFlags::None, MEMORY_LOCATION);
            return new (memory) T(std::forward<Args>(args)...);
        }

        template <typename T>
        void Delete(T *ptr) {
            if (ptr) {
                ptr->~T();
                MemoryManager::Get().GetDefaultAllocator()->Deallocate(
                    ptr, MEMORY_LOCATION);
            }
        }

    }  // namespace Memory
}  // namespace Engine
