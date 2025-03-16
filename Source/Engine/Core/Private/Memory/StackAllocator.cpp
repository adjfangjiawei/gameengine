
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
            markers.push(reinterpret_cast<size_t>(
                memory));  // Push initial marker with actual memory address
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

            // Calculate header position and aligned address
            void *headerPos = current;
            void *userPos =
                static_cast<char *>(headerPos) + sizeof(AllocationHeader);
            void *alignedAddress = AlignPointer(userPos, alignment);
            size_t adjustment = static_cast<char *>(alignedAddress) -
                                static_cast<char *>(userPos);

            // Check if we have enough space
            size_t totalRequired = sizeof(AllocationHeader) + adjustment + size;
            if (static_cast<char *>(memory) + totalSize <
                static_cast<char *>(current) + totalRequired) {
                LOG_ERROR("Stack allocator out of memory");
                return nullptr;
            }

            // Setup header
            AllocationHeader *header =
                static_cast<AllocationHeader *>(headerPos);
            header->size = size;
            header->adjustment = adjustment;
            header->prevMarker =
                markers
                    .top();  // Store current top marker before this allocation

            // Update current pointer (after allocation)
            current = static_cast<char *>(alignedAddress) + size;

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

            // Get header by going back by adjustment amount
            AllocationHeader *header = reinterpret_cast<AllocationHeader *>(
                static_cast<char *>(ptr) - sizeof(AllocationHeader) -
                reinterpret_cast<AllocationHeader *>(static_cast<char *>(ptr) -
                                                     sizeof(AllocationHeader))
                    ->adjustment);

            // Update stats
            stats.totalDeallocated += header->size;
            stats.currentUsage -= header->size;
            stats.deallocationCount++;

            // Reset current pointer to previous marker
            current = reinterpret_cast<void *>(header->prevMarker);

            // Pop markers until we've reached the previous state
            // (only if they're greater than where we're rewinding to)
            while (!markers.empty() && markers.top() > header->prevMarker) {
                markers.pop();
            }
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

            // Find the header by going back from the user pointer
            AllocationHeader *header = reinterpret_cast<AllocationHeader *>(
                static_cast<char *>(ptr) - sizeof(AllocationHeader) -
                reinterpret_cast<AllocationHeader *>(static_cast<char *>(ptr) -
                                                     sizeof(AllocationHeader))
                    ->adjustment);

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
            markers.push(reinterpret_cast<size_t>(
                memory));  // Push initial marker with actual memory address
            stats = MemoryStats{};
        }

        size_t StackAllocator::GetMarker() const {
            std::lock_guard<std::mutex> lock(mutex);
            return reinterpret_cast<size_t>(
                current);  // Return the current position as a marker
        }

        void StackAllocator::FreeToMarker(size_t marker) {
            std::lock_guard<std::mutex> lock(mutex);

            if (marker > reinterpret_cast<size_t>(current) ||
                marker < reinterpret_cast<size_t>(memory)) {
                LOG_ERROR("Invalid marker");
                return;
            }

            // Calculate freed memory amount
            size_t freedMemory = reinterpret_cast<size_t>(current) - marker;

            // Update current pointer to marker position
            current = reinterpret_cast<void *>(marker);

            // Pop markers until we reach a marker that's less than or equal to
            // our target
            while (!markers.empty() && markers.top() > marker) {
                markers.pop();
            }

            // Update stats (approximate as we don't track individual
            // allocations)
            if (freedMemory > stats.currentUsage) {
                stats.currentUsage = 0;
            } else {
                stats.currentUsage -= freedMemory;
            }
        }

    }  // namespace Memory
}  // namespace Engine
