
#pragma once

#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "CoreTypes.h"

namespace Engine {
    namespace Memory {

        /**
         * @brief Memory allocation statistics
         */
        struct MemoryStats {
            size_t totalAllocated = 0;     // Total allocated memory
            size_t totalDeallocated = 0;   // Total deallocated memory
            size_t currentUsage = 0;       // Current memory usage
            size_t peakUsage = 0;          // Peak memory usage
            size_t allocationCount = 0;    // Number of allocations
            size_t deallocationCount = 0;  // Number of deallocations
        };

        /**
         * @brief Memory allocation tracking information
         */
        struct AllocationInfo {
            void *address;         // Memory address
            size_t size;           // Allocation size
            const char *file;      // Source file
            int line;              // Source line
            const char *function;  // Function name
            const char *tag;       // Optional tag for grouping
        };

        /**
         * @brief Base interface for memory allocators
         */
        class IAllocator {
          public:
            virtual ~IAllocator() = default;

            virtual void *Allocate(
                size_t size, size_t alignment = alignof(std::max_align_t)) = 0;
            virtual void Deallocate(void *ptr) = 0;
            virtual size_t GetAllocationSize(void *ptr) const = 0;
            virtual bool OwnsAllocation(void *ptr) const = 0;
            virtual const MemoryStats &GetStats() const = 0;
            virtual void Reset() = 0;
        };

        /**
         * @brief Memory system configuration
         */
        struct MemoryConfig {
            size_t pageSize = 4096;  // System page size
            size_t defaultAlignment =
                alignof(std::max_align_t);    // Default alignment
            bool enableTracking = true;       // Enable allocation tracking
            bool enableGuards = true;         // Enable memory guards
            size_t guardSize = 8;             // Size of memory guards
            bool enableLeakDetection = true;  // Enable memory leak detection
        };

        /**
         * @brief Main memory system class
         */
        class MemorySystem {
          public:
            static MemorySystem &Get();

            void Initialize(const MemoryConfig &config = MemoryConfig());
            void Shutdown();

            // Allocator management
            void RegisterAllocator(const std::string &name,
                                   std::unique_ptr<IAllocator> allocator);
            IAllocator *GetAllocator(const std::string &name);
            void SetDefaultAllocator(const std::string &name);

            // Memory operations
            void *Allocate(size_t size,
                           const char *file = nullptr,
                           int line = 0,
                           const char *function = nullptr,
                           const char *tag = nullptr);
            void *AllocateAligned(size_t size,
                                  size_t alignment,
                                  const char *file = nullptr,
                                  int line = 0,
                                  const char *function = nullptr,
                                  const char *tag = nullptr);
            void Deallocate(void *ptr);

            // Memory tracking
            const MemoryStats &GetStats() const;
            std::vector<AllocationInfo> GetAllocationInfo() const;
            void DumpMemoryLeaks() const;
            void PrintStats() const;

            // Memory debugging
            bool ValidateAddress(void *ptr) const;
            bool ValidateGuards(void *ptr) const;
            void EnableTracking(bool enable);
            void EnableGuards(bool enable);

            // Memory utilities
            size_t GetPageSize() const;
            size_t GetDefaultAlignment() const;
            size_t GetAllocationSize(void *ptr) const;
            const char *GetAllocationTag(void *ptr) const;

          private:
            MemorySystem() = default;
            ~MemorySystem() = default;

            MemorySystem(const MemorySystem &) = delete;
            MemorySystem &operator=(const MemorySystem &) = delete;

            struct AllocationHeader {
                size_t size;
                const char *file;
                int line;
                const char *function;
                const char *tag;
                IAllocator *allocator;
            };

            void *AddHeader(void *ptr,
                            size_t size,
                            const char *file,
                            int line,
                            const char *function,
                            const char *tag);
            AllocationHeader *GetHeader(void *ptr) const;
            void AddGuards(void *ptr, size_t size);
            bool ValidateGuard(const void *guard) const;

          private:
            MemoryConfig m_config;
            std::unordered_map<std::string, std::unique_ptr<IAllocator>>
                m_allocators;
            IAllocator *m_defaultAllocator = nullptr;
            MemoryStats m_stats;
            mutable std::mutex m_mutex;
            bool m_initialized = false;
        };

// Helper macros for memory allocation
#define MEMORY_ALLOC(size)                        \
    Engine::Memory::MemorySystem::Get().Allocate( \
        size, __FILE__, __LINE__, __FUNCTION__)

#define MEMORY_ALLOC_ALIGNED(size, alignment)            \
    Engine::Memory::MemorySystem::Get().AllocateAligned( \
        size, alignment, __FILE__, __LINE__, __FUNCTION__)

#define MEMORY_ALLOC_TAGGED(size, tag)            \
    Engine::Memory::MemorySystem::Get().Allocate( \
        size, __FILE__, __LINE__, __FUNCTION__, tag)

#define MEMORY_FREE(ptr) Engine::Memory::MemorySystem::Get().Deallocate(ptr)

        // Custom deleters for smart pointers using the memory system
        template <typename T>
        struct MemoryDeleter {
            void operator()(T *ptr) {
                if (ptr) {
                    ptr->~T();
                    MEMORY_FREE(ptr);
                }
            }
        };

        // Helper functions for creating smart pointers using the memory system
        template <typename T, typename... Args>
        std::unique_ptr<T, MemoryDeleter<T>> MakeUnique(Args &&...args) {
            void *ptr = MEMORY_ALLOC(sizeof(T));
            T *obj = new (ptr) T(std::forward<Args>(args)...);
            return std::unique_ptr<T, MemoryDeleter<T>>(obj);
        }

        template <typename T, typename... Args>
        std::shared_ptr<T> MakeShared(Args &&...args) {
            void *ptr = MEMORY_ALLOC(sizeof(T));
            T *obj = new (ptr) T(std::forward<Args>(args)...);
            return std::shared_ptr<T>(obj, MemoryDeleter<T>());
        }

    }  // namespace Memory
}  // namespace Engine

// Legacy namespace for backward compatibility
namespace Engine {
    using MemorySystem = Engine::Memory::MemorySystem;
    using IAllocator = Engine::Memory::IAllocator;
    using MemoryConfig = Engine::Memory::MemoryConfig;
    using AllocationInfo = Engine::Memory::AllocationInfo;
    using MemoryStats = Engine::Memory::MemoryStats;
    using Engine::Memory::MakeShared;
    using Engine::Memory::MakeUnique;
    using Engine::Memory::MemoryDeleter;
}  // namespace Engine
