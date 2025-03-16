
#include "Core/Memory/MemoryManager.h"

#include <algorithm>

#include "Core/Assert.h"

namespace Engine {
    namespace Memory {

        MemoryManager &MemoryManager::Get() {
            static MemoryManager instance;
            return instance;
        }

        void MemoryManager::RegisterAllocator(const char *name,
                                              IAllocator *allocator) {
            ASSERT_MSG(name && allocator,
                       "Invalid allocator registration",
                       AssertType::Fatal);
            std::lock_guard<std::mutex> lock(mutex);

            auto result = allocators.emplace(name, allocator);
            ASSERT_MSG(result.second,
                       "Allocator already registered",
                       AssertType::Fatal);

            if (!defaultAllocator) {
                defaultAllocator = allocator;
            }
        }

        void MemoryManager::UnregisterAllocator(const char *name) {
            ASSERT_MSG(name, "Invalid allocator name", AssertType::Fatal);
            std::lock_guard<std::mutex> lock(mutex);

            auto it = allocators.find(name);
            if (it != allocators.end()) {
                if (it->second == defaultAllocator) {
                    defaultAllocator = nullptr;
                }
                allocators.erase(it);
            }
        }

        IAllocator *MemoryManager::GetAllocator(const char *name) {
            std::lock_guard<std::mutex> lock(mutex);

            auto it = allocators.find(name);
            return it != allocators.end() ? it->second : nullptr;
        }

        IAllocator *MemoryManager::GetDefaultAllocator() {
            std::lock_guard<std::mutex> lock(mutex);
            ASSERT_MSG(defaultAllocator,
                       "No default allocator set",
                       AssertType::Fatal);
            return defaultAllocator;
        }

        void MemoryManager::SetDefaultAllocator(const char *name) {
            std::lock_guard<std::mutex> lock(mutex);

            auto it = allocators.find(name);
            ASSERT_MSG(it != allocators.end(),
                       "Allocator not found",
                       AssertType::Fatal);
            defaultAllocator = it->second;
        }

        void MemoryManager::EnableTracking(bool enable) {
            trackingEnabled = enable;
        }

        bool MemoryManager::IsTrackingEnabled() const {
            return trackingEnabled;
        }

        MemoryStats MemoryManager::GetTotalStats() const {
            std::lock_guard<std::mutex> lock(mutex);

            MemoryStats total;
            for (const auto &pair : allocators) {
                const MemoryStats &stats = pair.second->GetStats();
                total.totalAllocated += stats.totalAllocated;
                total.totalDeallocated += stats.totalDeallocated;
                total.currentUsage += stats.currentUsage;
                total.peakUsage = std::max(total.peakUsage, stats.peakUsage);
                total.allocationCount += stats.allocationCount;
                total.deallocationCount += stats.deallocationCount;
            }
            return total;
        }

        MemoryStats MemoryManager::GetAllocatorStats(const char *name) const {
            std::lock_guard<std::mutex> lock(mutex);

            auto it = allocators.find(name);
            return it != allocators.end() ? it->second->GetStats()
                                          : MemoryStats{};
        }

        void MemoryManager::CheckLeaks() {
            std::lock_guard<std::mutex> lock(mutex);

            for (const auto &pair : allocators) {
                const MemoryStats &stats = pair.second->GetStats();
                if (stats.currentUsage > 0) {
                    LOG_WARNING(
                        "Memory leak detected in allocator '{}': {} bytes",
                        pair.first,
                        stats.currentUsage);
                }
            }
        }

        void MemoryManager::DumpLeaks() {
            std::lock_guard<std::mutex> lock(mutex);

            LOG_INFO("Memory Leak Report:");
            LOG_INFO("==================");

            for (const auto &pair : allocators) {
                const MemoryStats &stats = pair.second->GetStats();
                if (stats.currentUsage > 0) {
                    LOG_INFO("Allocator: {}", pair.first);
                    LOG_INFO("  Current Usage: {} bytes", stats.currentUsage);
                    LOG_INFO("  Total Allocated: {} bytes",
                             stats.totalAllocated);
                    LOG_INFO("  Total Deallocated: {} bytes",
                             stats.totalDeallocated);
                    LOG_INFO("  Peak Usage: {} bytes", stats.peakUsage);
                    LOG_INFO("  Allocation Count: {}", stats.allocationCount);
                    LOG_INFO("  Deallocation Count: {}",
                             stats.deallocationCount);
                    LOG_INFO("------------------");
                }
            }
        }

        void MemoryManager::DumpStats() {
            std::lock_guard<std::mutex> lock(mutex);

            LOG_INFO("Memory Statistics:");
            LOG_INFO("=================");

            for (const auto &pair : allocators) {
                const MemoryStats &stats = pair.second->GetStats();
                LOG_INFO("Allocator: {}", pair.first);
                LOG_INFO("  Current Usage: {} bytes", stats.currentUsage);
                LOG_INFO("  Peak Usage: {} bytes", stats.peakUsage);
                LOG_INFO("  Total Allocated: {} bytes", stats.totalAllocated);
                LOG_INFO("  Total Deallocated: {} bytes",
                         stats.totalDeallocated);
                LOG_INFO("  Allocation Count: {}", stats.allocationCount);
                LOG_INFO("  Deallocation Count: {}", stats.deallocationCount);
                LOG_INFO("------------------");
            }

            MemoryStats total = GetTotalStats();
            LOG_INFO("Total Memory Usage:");
            LOG_INFO("  Current: {} bytes", total.currentUsage);
            LOG_INFO("  Peak: {} bytes", total.peakUsage);
        }

        void MemoryManager::ValidateHeap() {
            // This is a placeholder for heap validation
            // In a real implementation, this would perform thorough heap checks
            LOG_INFO("Heap validation not implemented");
        }

    }  // namespace Memory
}  // namespace Engine

// Global new/delete operators
void *operator new(size_t size) { return Engine::Memory::Allocate(size); }

void *operator new[](size_t size) { return Engine::Memory::Allocate(size); }

void operator delete(void *ptr) noexcept { Engine::Memory::Deallocate(ptr); }

void operator delete[](void *ptr) noexcept { Engine::Memory::Deallocate(ptr); }

void *operator new(size_t size, const char *file, int line) {
    return Engine::Memory::Allocate(size,
                                    Engine::Memory::DEFAULT_ALIGNMENT,
                                    Engine::Memory::AllocFlags::None,
                                    {file, line, ""});
}

void *operator new[](size_t size, const char *file, int line) {
    return Engine::Memory::Allocate(size,
                                    Engine::Memory::DEFAULT_ALIGNMENT,
                                    Engine::Memory::AllocFlags::None,
                                    {file, line, ""});
}

void operator delete(void *ptr, const char *file, int line) noexcept {
    Engine::Memory::Deallocate(ptr, {file, line, ""});
}

void operator delete[](void *ptr, const char *file, int line) noexcept {
    Engine::Memory::Deallocate(ptr, {file, line, ""});
}
