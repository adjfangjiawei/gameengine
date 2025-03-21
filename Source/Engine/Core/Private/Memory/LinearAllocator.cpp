
#include "Memory/LinearAllocator.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "Assert.h"
#include "Log/LogSystem.h"

namespace Engine {
    namespace Memory {

        LinearAllocator::LinearAllocator(const Config &config)
            : totalSize(config.totalSize), allowReset(config.allowReset) {
            ASSERT_MSG(totalSize > 0,
                       "Total size must be greater than 0",
                       AssertType::Fatal);

            // Allocate memory block
            memory = std::malloc(totalSize);
            ASSERT_MSG(
                memory, "Failed to allocate memory block", AssertType::Fatal);

            current = memory;
        }

        LinearAllocator::~LinearAllocator() {
            if (memory) {
                std::free(memory);
                memory = nullptr;
                current = nullptr;
            }
        }

        void *LinearAllocator::Allocate(size_t size,
                                        size_t alignment,
                                        AllocFlags flags,
                                        const MemoryLocation &location) {
            UNUSED(flags);     // Mark flags as unused
            UNUSED(location);  // Mark location as unused

            std::lock_guard<std::mutex> lock(mutex);

            // Calculate aligned address and adjustment
            size_t adjustment = 0;
            void *alignedAddress = AlignPointer(current, alignment);
            adjustment = static_cast<char *>(alignedAddress) -
                         static_cast<char *>(current);
            ASSERT_MSG(
                adjustment == (size_t)(static_cast<char *>(alignedAddress) -
                                       static_cast<char *>(current)),
                "Adjustment should be equal to the difference between the "
                "aligned address and the current address",
                AssertType::Error);
            current = alignedAddress;

            // Check if we have enough space
            size_t totalRequired = size + adjustment + sizeof(AllocationHeader);
            if (static_cast<char *>(memory) + totalSize <
                static_cast<char *>(current) + totalRequired) {
                LOG_ERROR("Linear allocator out of memory");
                return nullptr;
            }

            // Setup header
            AllocationHeader *header = static_cast<AllocationHeader *>(current);
            header->size = size;
            header->adjustment = adjustment;

            // Update current pointer
            current = static_cast<char *>(alignedAddress) + size;

            // Update stats
            stats.totalAllocated += size;
            stats.currentUsage += size;
            stats.allocationCount++;
            stats.peakUsage = std::max(stats.peakUsage, stats.currentUsage);

            // Zero memory if requested
            void *userPtr =
                static_cast<char *>(alignedAddress) + sizeof(AllocationHeader);
            if ((flags & AllocFlags::ZeroMemory) == AllocFlags::ZeroMemory) {
                std::memset(userPtr, 0, size);
            }

            return userPtr;
        }

        void LinearAllocator::Deallocate(void *ptr,
                                         const MemoryLocation &location) {
            UNUSED(ptr);       // Mark ptr as unused
            UNUSED(location);  // Mark location as unused

            // Linear allocator doesn't support individual deallocations
            // Memory is freed all at once when Reset() is called
            if (!allowReset) {
                LOG_WARNING(
                    "Attempted to deallocate from linear allocator with reset "
                    "disabled");
            }
        }

        void *LinearAllocator::AllocateTracked(size_t size,
                                               size_t alignment,
                                               const char *typeName,
                                               AllocFlags flags,
                                               const MemoryLocation &location) {
            UNUSED(typeName);  // Mark typeName as unused

            return Allocate(size, alignment, flags, location);
        }

        size_t LinearAllocator::GetAllocationSize(void *ptr) const {
            if (!OwnsAllocation(ptr)) {
                return 0;
            }

            // The header is located at (userPtr - adjustment -
            // sizeof(AllocationHeader)) First, get the aligned user pointer
            char *userPtr = static_cast<char *>(ptr);

            // Find the header by going backwards by one byte to get a value we
            // can use to determine the adjustment
            AllocationHeader *header = reinterpret_cast<AllocationHeader *>(
                userPtr - sizeof(AllocationHeader));

            // Now use the adjustment value from the header to get the actual
            // header location
            header = reinterpret_cast<AllocationHeader *>(
                reinterpret_cast<char *>(header) - header->adjustment);

            return header->size;
        }

        bool LinearAllocator::OwnsAllocation(void *ptr) const {
            return ptr >= memory && ptr < current;
        }

        const MemoryStats &LinearAllocator::GetStats() const { return stats; }

        void LinearAllocator::Reset() {
            if (!allowReset) {
                LOG_ERROR("Reset not allowed for this linear allocator");
                return;
            }

            std::lock_guard<std::mutex> lock(mutex);
            current = memory;
            stats = MemoryStats{};
        }

    }  // namespace Memory
}  // namespace Engine
