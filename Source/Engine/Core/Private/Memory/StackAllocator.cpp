
#include "Memory/StackAllocator.h"

#include <cstring>

#include "Assert.h"
#include "Log/LogSystem.h"

namespace Engine {
    namespace Memory {

        StackAllocator::StackAllocator(const Config &config)
            : totalSize(config.totalSize) {
            ASSERT_MSG(totalSize > 0,
                       "Total size must be greater than 0",
                       AssertType::Fatal);

            // Allocate memory block
            memory = std::malloc(totalSize);
            ASSERT_MSG(
                memory, "Failed to allocate memory block", AssertType::Fatal);

            current = memory;
            markers.push(0);  // Push initial marker
        }

        StackAllocator::~StackAllocator() {
            if (memory) {
                std::free(memory);
                memory = nullptr;
                current = nullptr;
            }
        }

        void *StackAllocator::Allocate(size_t size,
                                       size_t alignment,
                                       AllocFlags flags,
                                       const MemoryLocation &location) {
            UNUSED(location);  // Mark location as unused

            std::lock_guard<std::mutex> lock(mutex);

            // Calculate aligned address and adjustment
            size_t adjustment = 0;
            void *alignedAddress = AlignPointer(current, alignment);
            adjustment = static_cast<char *>(alignedAddress) -
                         static_cast<char *>(current);

            // Check if we have enough space
            size_t totalRequired = size + adjustment + sizeof(AllocationHeader);
            if (static_cast<char *>(memory) + totalSize <
                static_cast<char *>(current) + totalRequired) {
                LOG_ERROR("Stack allocator out of memory");
                return nullptr;
            }

            // Setup header
            AllocationHeader *header = static_cast<AllocationHeader *>(current);
            header->size = size;
            header->adjustment = adjustment;
            header->prevMarker = markers.top();

            // Update current pointer
            current = static_cast<char *>(alignedAddress) + size;
            markers.push(reinterpret_cast<size_t>(current));

            // Update stats
            stats.totalAllocated += size;
            stats.currentUsage += size;
            stats.allocationCount++;
            stats.peakUsage = std::max(stats.peakUsage, stats.currentUsage);

            // Zero memory if requested
            void *userPtr = alignedAddress;
            if ((flags & AllocFlags::ZeroMemory) == AllocFlags::ZeroMemory) {
                std::memset(userPtr, 0, size);
            }

            return userPtr;
        }

        void StackAllocator::Deallocate(void *ptr,
                                        const MemoryLocation &location) {
            UNUSED(location);  // Mark location as unused

            if (!ptr || !OwnsAllocation(ptr)) {
                return;
            }

            std::lock_guard<std::mutex> lock(mutex);

            // Get header
            AllocationHeader *header = reinterpret_cast<AllocationHeader *>(
                static_cast<char *>(ptr) - sizeof(AllocationHeader));

            // Update stats
            stats.totalDeallocated += header->size;
            stats.currentUsage -= header->size;
            stats.deallocationCount++;

            // Reset current pointer to previous marker
            current = reinterpret_cast<void *>(header->prevMarker);
            markers.pop();
        }

        void *StackAllocator::AllocateTracked(size_t size,
                                              size_t alignment,
                                              const char *typeName,
                                              AllocFlags flags,
                                              const MemoryLocation &location) {
            UNUSED(typeName);  // Mark typeName as unused

            return Allocate(size, alignment, flags, location);
        }

        size_t StackAllocator::GetAllocationSize(void *ptr) const {
            if (!OwnsAllocation(ptr)) {
                return 0;
            }

            AllocationHeader *header = reinterpret_cast<AllocationHeader *>(
                static_cast<char *>(ptr) - sizeof(AllocationHeader));
            return header->size;
        }

        bool StackAllocator::OwnsAllocation(void *ptr) const {
            return ptr >= memory && ptr < current;
        }

        const MemoryStats &StackAllocator::GetStats() const { return stats; }

        void StackAllocator::Reset() {
            std::lock_guard<std::mutex> lock(mutex);
            current = memory;
            while (!markers.empty()) markers.pop();
            markers.push(0);  // Push initial marker
            stats = MemoryStats{};
        }

        size_t StackAllocator::GetMarker() const {
            std::lock_guard<std::mutex> lock(mutex);
            return markers.top();
        }

        void StackAllocator::FreeToMarker(size_t marker) {
            std::lock_guard<std::mutex> lock(mutex);

            if (marker > reinterpret_cast<size_t>(current) ||
                marker < reinterpret_cast<size_t>(memory)) {
                LOG_ERROR("Invalid marker");
                return;
            }

            // Pop markers until we reach the target
            while (!markers.empty() && markers.top() > marker) {
                markers.pop();
            }

            // Update current pointer
            current = reinterpret_cast<void *>(marker);

            // Update stats (approximate as we don't track individual
            // allocations)
            size_t freedMemory = reinterpret_cast<char *>(current) -
                                 reinterpret_cast<char *>(marker);
            stats.currentUsage -= freedMemory;
        }

    }  // namespace Memory
}  // namespace Engine
