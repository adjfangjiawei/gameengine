
#include "Core/Memory/HeapAllocator.h"

#include <math.h>

#include <algorithm>
#include <chrono>
#include <cstring>

#include "Core/Assert.h"
#include "Core/Log/LogSystem.h"
#include "Core/Platform.h"
#if PLATFORM_WINDOWS
#include <dbghelp.h>
#include <windows.h>
#pragma comment(lib, "dbghelp.lib")
#else
#include <execinfo.h>
#endif
using namespace std::string_literals;

namespace Engine {
    namespace Memory {

        HeapAllocator::HeapAllocator(const Config &config) : config(config) {
#if PLATFORM_WINDOWS
            SymInitialize(GetCurrentProcess(), nullptr, TRUE);
#endif
            lastDefragmentationTime = std::chrono::steady_clock::now();
        }

        HeapAllocator::~HeapAllocator() {
            CheckForLeaks();
#if PLATFORM_WINDOWS
            SymCleanup(GetCurrentProcess());
#endif
        }

        void *HeapAllocator::Allocate(size_t size,
                                      size_t alignment,
                                      AllocFlags flags,
                                      const MemoryLocation &location) {
            return AllocateInternal(size, alignment, nullptr, flags, location);
        }

        void HeapAllocator::Deallocate(void *ptr,
                                       const MemoryLocation &location) {
            if (!ptr) return;

            std::unique_lock<std::shared_mutex> lock(mutex);

            auto it = allocations.find(ptr);
            if (it == allocations.end()) {
                LOG_ERROR("Attempting to deallocate unknown pointer at {}",
                          location.file);
                return;
            }

            AllocationInfo &info = it->second;

            // Check for double free
            if (info.header.isFreed) {
                LOG_ERROR(
                    "Double free detected at {} (originally freed at {}:{})",
                    location.file,
                    info.header.location.file,
                    info.header.location.line);
                return;
            }

            // Validate header checksum
            ValidateHeader(info.header);

            // Validate guard bytes if enabled
            if (config.enableGuardBytes) {
                ValidateGuardBytes(info);
            }

            // Update stats
            detailedStats.totalDeallocated += info.header.size;
            detailedStats.currentUsage -= info.header.size;
            detailedStats.deallocationCount++;

            // Update type-specific stats
            if (info.header.typeName) {
                detailedStats.allocationsByType[info.header.typeName] -=
                    info.header.size;
            }
            detailedStats.allocationsByFile[info.header.location.file] -=
                info.header.size;

            // Mark as freed and fill with pattern
            info.header.isFreed = true;
            if (config.enableDebugInfo) {
                FillFreedMemory(ptr, info.header.size);
            }

            // Free memory
            std::free(info.originalPtr);
            allocations.erase(it);

            // Check if defragmentation is needed
            CheckAndTriggerDefragmentation();
        }

        void *HeapAllocator::AllocateTracked(size_t size,
                                             size_t alignment,
                                             const char *typeName,
                                             AllocFlags flags,
                                             const MemoryLocation &location) {
            return AllocateInternal(size, alignment, typeName, flags, location);
        }

        void *HeapAllocator::AllocateInternal(size_t size,
                                              size_t alignment,
                                              const char *typeName,
                                              AllocFlags flags,
                                              const MemoryLocation &location) {
            std::unique_lock<std::shared_mutex> lock(mutex);

            // Calculate sizes with improved alignment handling
            size_t headerSize =
                AlignUp(sizeof(AllocationHeader),
                        std::max(alignment, alignof(AllocationHeader)));
            size_t guardSize = config.enableGuardBytes ? config.guardSize : 0;
            size_t totalSize = headerSize + size + (guardSize * 2);
            size_t alignedTotalSize = AlignUp(totalSize, alignment);

            // Allocate memory
            void *originalPtr = std::malloc(alignedTotalSize);
            if (!originalPtr) {
                LOG_ERROR("Failed to allocate memory of size {} at {}",
                          size,
                          location.file);
                return nullptr;
            }

            // Calculate aligned pointers
            void *headerPtr = AlignPointer(
                originalPtr, std::max(alignment, alignof(AllocationHeader)));
            void *userPtr = static_cast<char *>(headerPtr) + headerSize;

            // Ensure user pointer is properly aligned
            if (!IsAligned(userPtr, alignment)) {
                userPtr = AlignPointer(userPtr, alignment);
                headerPtr = static_cast<char *>(userPtr) - headerSize;
            }

            // Calculate padding sizes
            size_t paddingFront = static_cast<char *>(headerPtr) -
                                  static_cast<char *>(originalPtr);
            size_t paddingBack = alignedTotalSize - (paddingFront + headerSize +
                                                     size + (guardSize * 2));

            // Setup guard bytes if enabled
            if (config.enableGuardBytes) {
                SetGuardBytes(static_cast<char *>(userPtr) - guardSize,
                              guardSize);
                SetGuardBytes(static_cast<char *>(userPtr) + size, guardSize);
            }

            // Initialize header
            AllocationHeader *header =
                static_cast<AllocationHeader *>(headerPtr);
            header->size = size;
            header->alignment = alignment;
            header->typeName = typeName;
            header->location = location;
            header->timestamp =
                std::chrono::steady_clock::now().time_since_epoch().count();
            header->threadId = std::this_thread::get_id();
            header->backTraceSize = 0;
            header->isFreed = false;
            header->allocCount = ++nextAllocCount;
            header->paddingFront = paddingFront;
            header->paddingBack = paddingBack;

            if (config.enableDebugInfo) {
                CaptureBackTrace(*header);
            }

            // Calculate and store header checksum
            header->checksum = CalculateHeaderChecksum(*header);

            // Update detailed stats
            detailedStats.totalAllocated += size;
            detailedStats.currentUsage += size;
            detailedStats.allocationCount++;
            detailedStats.peakUsage =
                std::max(detailedStats.peakUsage, detailedStats.currentUsage);
            detailedStats.headerOverhead += headerSize;
            detailedStats.guardOverhead += (guardSize * 2);
            detailedStats.wastedMemory += (paddingFront + paddingBack);

            if (typeName) {
                detailedStats.allocationsByType[typeName] += size;
            }
            detailedStats.allocationsByFile[location.file] += size;

            // Store allocation info
            if (config.trackAllocations) {
                AllocationInfo info;
                info.header = *header;
                info.actualSize = alignedTotalSize;
                info.originalPtr = originalPtr;
                info.paddingSize = paddingFront + paddingBack;
                info.allocationTime = std::chrono::steady_clock::now();
                info.needsDefragmentation = false;
                allocations[userPtr] = info;
            }

            // Zero memory if requested
            if ((flags & AllocFlags::ZeroMemory) == AllocFlags::ZeroMemory) {
                std::memset(userPtr, 0, size);
            }

            return userPtr;
        }

        size_t HeapAllocator::GetAllocationSize(void *ptr) const {
            std::shared_lock<std::shared_mutex> lock(mutex);

            auto it = allocations.find(ptr);
            if (it == allocations.end()) {
                return 0;
            }

            return it->second.header.size;
        }

        bool HeapAllocator::OwnsAllocation(void *ptr) const {
            std::shared_lock<std::shared_mutex> lock(mutex);
            return allocations.find(ptr) != allocations.end();
        }

        const MemoryStats &HeapAllocator::GetStats() const {
            std::shared_lock<std::shared_mutex> lock(mutex);
            return detailedStats;
        }

        void HeapAllocator::Reset() {
            std::unique_lock<std::shared_mutex> lock(mutex);

            // Free all allocations
            for (auto &pair : allocations) {
                std::free(pair.second.originalPtr);
            }

            allocations.clear();
            detailedStats.Clear();
            nextAllocCount = 0;
            totalDefragmentationCount = 0;
            lastDefragmentationTime = std::chrono::steady_clock::now();
        }

        void HeapAllocator::CaptureBackTrace(AllocationHeader &header) {
#if PLATFORM_WINDOWS
            header.backTraceSize = CaptureStackBackTrace(
                1,
                static_cast<DWORD>(config.debugBackTraceDepth),
                header.backTrace,
                nullptr);
#else
            header.backTraceSize = static_cast<size_t>(
                backtrace(header.backTrace,
                          static_cast<int>(config.debugBackTraceDepth)));
#endif
        }

        void HeapAllocator::ValidateGuardBytes(
            const AllocationInfo &info) const {
            if (!config.enableGuardBytes) {
                return;
            }

            const uint8_t *frontGuard =
                static_cast<const uint8_t *>(info.originalPtr) +
                info.header.paddingFront + sizeof(AllocationHeader);
            const uint8_t *backGuard = frontGuard + info.header.size;

            for (size_t i = 0; i < config.guardSize; ++i) {
                if (frontGuard[i] != GUARD_PATTERN ||
                    backGuard[i] != GUARD_PATTERN) {
                    LOG_ERROR("Memory corruption detected at {}:{}",
                              info.header.location.file,
                              info.header.location.line);
                    ASSERT(false, "Memory corruption detected");
                }
            }
        }

        void HeapAllocator::SetGuardBytes(void *ptr, size_t size) {
            std::memset(ptr, GUARD_PATTERN, size);
        }

        uint32_t HeapAllocator::CalculateHeaderChecksum(
            const AllocationHeader &header) const {
            // Simple FNV-1a hash
            uint32_t hash = 2166136261u;
            const uint8_t *data = reinterpret_cast<const uint8_t *>(&header);
            size_t size = sizeof(AllocationHeader) - sizeof(header.checksum);

            for (size_t i = 0; i < size; ++i) {
                hash ^= data[i];
                hash *= 16777619u;
            }

            return hash;
        }

        void HeapAllocator::ValidateHeader(
            const AllocationHeader &header) const {
            uint32_t expectedChecksum = header.checksum;
            uint32_t actualChecksum = CalculateHeaderChecksum(header);

            if (expectedChecksum != actualChecksum) {
                LOG_ERROR("Memory header corruption detected at {}:{}",
                          header.location.file,
                          header.location.line);
                ASSERT(false, "Memory header corruption detected");
            }
        }

        void HeapAllocator::FillFreedMemory(void *ptr, size_t size) {
            std::memset(ptr, FREED_PATTERN, size);
        }

        void HeapAllocator::CheckForLeaks() const {
            std::shared_lock<std::shared_mutex> lock(mutex);

            if (allocations.empty()) {
                return;
            }

            size_t totalLeaked = 0;
            LOG_ERROR("Memory leaks detected:");

            for (const auto &pair : allocations) {
                const AllocationInfo &info = pair.second;
                totalLeaked += info.header.size;

                LOG_ERROR("  Leak of {} bytes at {}:{}",
                          info.header.size,
                          info.header.location.file,
                          info.header.location.line);

                if (info.header.typeName) {
                    LOG_ERROR("    Type: {}", info.header.typeName);
                }

                if (config.enableDebugInfo && info.header.backTraceSize > 0) {
#if PLATFORM_WINDOWS
                    SYMBOL_INFO *symbol = (SYMBOL_INFO *)calloc(
                        sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
                    symbol->MaxNameLen = 255;
                    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

                    for (size_t i = 0; i < info.header.backTraceSize; i++) {
                        SymFromAddr(GetCurrentProcess(),
                                    (DWORD64)(info.header.backTrace[i]),
                                    0,
                                    symbol);
                        LOG_ERROR("    {}: {}", i, symbol->Name);
                    }

                    free(symbol);
#else
                    char **symbols = backtrace_symbols(
                        info.header.backTrace,
                        static_cast<int>(info.header.backTraceSize));

                    if (symbols) {
                        for (size_t i = 0; i < info.header.backTraceSize; i++) {
                            LOG_ERROR("    {}: {}", i, symbols[i]);
                        }
                        free(symbols);
                    }
#endif
                }
            }

            LOG_ERROR("Total memory leaked: {} bytes", totalLeaked);
        }

        void HeapAllocator::CheckAndTriggerDefragmentation() {
            if (!config.enableDefragmentation) {
                return;
            }

            auto now = std::chrono::steady_clock::now();
            auto timeSinceLastDefrag =
                std::chrono::duration_cast<std::chrono::seconds>(
                    now - lastDefragmentationTime)
                    .count();

            // Only check for defragmentation every 60 seconds
            if (timeSinceLastDefrag < 60) {
                return;
            }

            FragmentationInfo info = AnalyzeFragmentationInternal();
            if (info.fragmentationRatio * 100 >=
                config.defragmentationThreshold) {
                Defragment();
            }
        }

        void HeapAllocator::Defragment() {
            std::unique_lock<std::shared_mutex> lock(mutex);

            // Sort allocations by address for optimal defragmentation
            std::vector<std::pair<void *, AllocationInfo *>> sortedAllocations;
            for (auto &pair : allocations) {
                sortedAllocations.push_back({pair.first, &pair.second});
            }
            std::sort(sortedAllocations.begin(), sortedAllocations.end());

            size_t defragmentedCount = 0;
            for (size_t i = 0; i < sortedAllocations.size(); ++i) {
                AllocationInfo *info = sortedAllocations[i].second;
                if (TryDefragment(*info)) {
                    defragmentedCount++;
                }
            }

            if (defragmentedCount > 0) {
                totalDefragmentationCount++;
                LOG_INFO("Defragmented {} allocations", defragmentedCount);
                UpdateDetailedStats();
            }

            lastDefragmentationTime = std::chrono::steady_clock::now();
        }

        bool HeapAllocator::TryDefragment(AllocationInfo &info) {
            if (info.header.isFreed || !info.needsDefragmentation) {
                return false;
            }

            // Allocate new contiguous block
            void *newPtr = std::malloc(info.actualSize);
            if (!newPtr) {
                return false;
            }

            // Copy data to new location
            std::memcpy(newPtr, info.originalPtr, info.actualSize);

            // Update pointers
            void *oldUserPtr = static_cast<char *>(info.originalPtr) +
                               info.header.paddingFront +
                               sizeof(AllocationHeader);
            void *newUserPtr = static_cast<char *>(newPtr) +
                               info.header.paddingFront +
                               sizeof(AllocationHeader);

            // Update allocation map
            allocations.erase(oldUserPtr);
            info.originalPtr = newPtr;
            allocations[newUserPtr] = info;

            // Free old memory
            std::free(info.originalPtr);

            info.needsDefragmentation = false;
            return true;
        }

        void HeapAllocator::UpdateDetailedStats() {
            FragmentationInfo fragInfo = AnalyzeFragmentationInternal();
            detailedStats.fragmentationRatio = fragInfo.fragmentationRatio;
            detailedStats.fragmentedMemory = static_cast<size_t>(
                detailedStats.currentUsage * fragInfo.fragmentationRatio);
        }

        HeapAllocator::FragmentationInfo
        HeapAllocator::AnalyzeFragmentationInternal() const {
            std::shared_lock<std::shared_mutex> lock(mutex);

            FragmentationInfo info{};
            if (allocations.empty()) {
                return info;
            }

            // Sort allocations by address
            std::vector<std::pair<void *, const AllocationInfo *>>
                sortedAllocations;
            for (const auto &pair : allocations) {
                sortedAllocations.push_back({pair.first, &pair.second});
            }
            std::sort(sortedAllocations.begin(), sortedAllocations.end());

            // Calculate fragments
            size_t totalGapSize = 0;
            size_t fragmentCount = 0;

            for (size_t i = 1; i < sortedAllocations.size(); ++i) {
                const char *prevEnd =
                    static_cast<const char *>(sortedAllocations[i - 1].first) +
                    sortedAllocations[i - 1].second->header.size;
                const char *currStart =
                    static_cast<const char *>(sortedAllocations[i].first);

                if (currStart > prevEnd) {
                    size_t gap = currStart - prevEnd;
                    totalGapSize += gap;
                    fragmentCount++;

                    info.fragmentSizes.push_back({gap, 1});
                    info.largestFragment = std::max(info.largestFragment, gap);
                    if (info.smallestFragment == 0) {
                        info.smallestFragment = gap;
                    } else {
                        info.smallestFragment =
                            std::min(info.smallestFragment, gap);
                    }
                }
            }

            info.totalFragments = fragmentCount;
            info.averageFragmentSize =
                fragmentCount > 0 ? totalGapSize / fragmentCount : 0;
            info.fragmentationRatio =
                static_cast<double>(totalGapSize) /
                static_cast<double>(detailedStats.currentUsage + totalGapSize);

            return info;
        }

        const HeapAllocator::DetailedMemoryStats &
        HeapAllocator::GetDetailedStats() const {
            std::shared_lock<std::shared_mutex> lock(mutex);
            return detailedStats;
        }

        void HeapAllocator::SetDefragmentationThreshold(size_t percentage) {
            std::unique_lock<std::shared_mutex> lock(mutex);
            config.defragmentationThreshold = std::min(percentage, size_t(100));
        }

        double HeapAllocator::GetFragmentationRatio() const {
            std::shared_lock<std::shared_mutex> lock(mutex);
            return detailedStats.fragmentationRatio;
        }

        void HeapAllocator::DumpAllocationInfo() const {
            std::shared_lock<std::shared_mutex> lock(mutex);

            LOG_INFO("=== Memory Allocation Information ===");
            LOG_INFO("Total allocations: {}", detailedStats.allocationCount);
            LOG_INFO("Current memory usage: {} bytes",
                     detailedStats.currentUsage);
            LOG_INFO("Peak memory usage: {} bytes", detailedStats.peakUsage);
            LOG_INFO("Memory overhead: {} bytes",
                     detailedStats.headerOverhead +
                         detailedStats.guardOverhead +
                         detailedStats.wastedMemory);
            LOG_INFO("Fragmentation ratio: {:.2f}%",
                     detailedStats.fragmentationRatio * 100);

            LOG_INFO("\nAllocation by type:");
            for (const auto &pair : detailedStats.allocationsByType) {
                LOG_INFO("  {}: {} bytes", pair.first, pair.second);
            }

            LOG_INFO("\nAllocation by file:");
            for (const auto &pair : detailedStats.allocationsByFile) {
                LOG_INFO("  {}: {} bytes", pair.first, pair.second);
            }

            if (config.enableDebugInfo) {
                LOG_INFO("\nDetailed allocation list:");
                for (const auto &pair : allocations) {
                    const AllocationInfo &info = pair.second;
                    LOG_INFO("  Address: {:p}, Size: {}, Type: {}",
                             pair.first,
                             info.header.size,
                             info.header.typeName ? info.header.typeName
                                                  : "Unknown");
                }
            }
        }

        void HeapAllocator::ValidateHeap() const {
            std::shared_lock<std::shared_mutex> lock(mutex);

            for (const auto &pair : allocations) {
                const AllocationInfo &info = pair.second;

                // Validate header
                ValidateHeader(info.header);

                // Validate guard bytes
                if (config.enableGuardBytes) {
                    ValidateGuardBytes(info);
                }

                // Validate alignment
                if (!IsAligned(pair.first, info.header.alignment)) {
                    LOG_ERROR("Memory alignment violation detected at {}:{}",
                              info.header.location.file,
                              info.header.location.line);
                    ASSERT(false, "Memory alignment violation detected");
                }
            }
        }

    }  // namespace Memory
}  // namespace Engine
