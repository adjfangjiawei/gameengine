
#include "Memory/MemoryDebugger.h"

#include <algorithm>
#include <chrono>
#include <cstring>

#include "Assert.h"
#include "Log/LogSystem.h"
#include "Platform.h"

#if PLATFORM_WINDOWS
#include <dbghelp.h>
#include <windows.h>
#pragma comment(lib, "dbghelp.lib")
#else
#include <execinfo.h>
#endif

namespace Engine {
    namespace Memory {

        MemoryDebugger &MemoryDebugger::Get() {
            static MemoryDebugger instance;
            return instance;
        }

        void MemoryDebugger::Initialize(const Config &config) {
            std::lock_guard<std::mutex> lock(mutex);
            this->config = config;
            allocations.clear();
            accessHistory.clear();
            accessHistory.reserve(config.maxAccessHistory);

#if PLATFORM_WINDOWS
            SymInitialize(GetCurrentProcess(), nullptr, TRUE);
#endif
        }

        void MemoryDebugger::Shutdown() {
            std::lock_guard<std::mutex> lock(mutex);
            DumpLeakReport();
            allocations.clear();
            accessHistory.clear();

#if PLATFORM_WINDOWS
            SymCleanup(GetCurrentProcess());
#endif
        }

        void *MemoryDebugger::OnAllocation(void *ptr,
                                           size_t size,
                                           size_t alignment,
                                           const char *typeName,
                                           const MemoryLocation &location) {
            if (!ptr || !config.enableBoundaryChecking) return ptr;

            std::lock_guard<std::mutex> lock(mutex);

            // Place guard bands
            void *guardedPtr = PlaceGuardBands(ptr, size);

            // Fill new memory with pattern
            if (config.enablePatternFill) {
                FillPattern(guardedPtr, size, NewPattern);
            }

            // Record allocation
            AllocationRecord record;
            record.address = guardedPtr;
            record.size = size;
            record.alignment = alignment;
            record.typeName = typeName;
            record.location = location;
            record.timestamp =
                std::chrono::steady_clock::now().time_since_epoch().count();
            record.backTraceSize = 0;

            CaptureStackTrace(record.backTrace, record.backTraceSize);
            allocations[guardedPtr] = record;

            return guardedPtr;
        }

        void MemoryDebugger::OnDeallocation(
            void *ptr, [[maybe_unused]] const MemoryLocation &location) {
            if (!ptr) return;

            std::lock_guard<std::mutex> lock(mutex);

            auto it = allocations.find(ptr);
            if (it == allocations.end()) {
                LOG_ERROR("Attempting to deallocate unknown pointer: {}", ptr);
                return;
            }

            const AllocationRecord &record = it->second;

            // Validate guard bands
            if (config.enableBoundaryChecking) {
                ValidateGuardBands(record);
            }

            // Fill freed memory with pattern
            if (config.enablePatternFill) {
                FillPattern(ptr, record.size, DeletePattern);
            }

            allocations.erase(it);
        }

        void MemoryDebugger::OnAccess(void *ptr,
                                      size_t size,
                                      bool isWrite,
                                      const MemoryLocation &location) {
            if (!ptr || !config.enableAccessTracking) return;

            std::lock_guard<std::mutex> lock(mutex);

            // Record access
            AccessRecord record;
            record.address = ptr;
            record.size = size;
            record.isWrite = isWrite;
            record.timestamp =
                std::chrono::steady_clock::now().time_since_epoch().count();
            record.file = location.file.c_str();
            record.line = location.line;
            record.backTraceSize = 0;

            CaptureStackTrace(record.backTrace, record.backTraceSize);

            // Add to circular buffer
            if (accessHistory.size() >= config.maxAccessHistory) {
                accessHistory.erase(accessHistory.begin());
            }
            accessHistory.push_back(record);

            // Validate access is within bounds
            auto it = allocations.find(ptr);
            if (it != allocations.end()) {
                const AllocationRecord &alloc = it->second;
                if (static_cast<char *>(ptr) + size >
                    static_cast<char *>(alloc.address) + alloc.size) {
                    LOG_ERROR(
                        "Memory access out of bounds: {} + {} exceeds "
                        "allocation of {}",
                        ptr,
                        size,
                        alloc.size);
                }
            }
        }

        void MemoryDebugger::ValidateAllAllocations() {
            std::lock_guard<std::mutex> lock(mutex);

            for (const auto &pair : allocations) {
                const AllocationRecord &record = pair.second;

                // Check guard bands
                if (config.enableBoundaryChecking) {
                    ValidateGuardBands(record);
                }

                // Validate alignment
                if (!IsAligned(record.address, record.alignment)) {
                    LOG_ERROR("Memory misaligned: {} (required {})",
                              record.address,
                              record.alignment);
                }
            }
        }

        void MemoryDebugger::DumpAccessHistory() const {
            std::lock_guard<std::mutex> lock(mutex);

            LOG_INFO("Memory Access History:");
            LOG_INFO("=====================");

            for (const auto &record : accessHistory) {
                LOG_INFO("{} access at {} (size: {}) from {}:{}",
                         record.isWrite ? "Write" : "Read",
                         record.address,
                         record.size,
                         record.file,
                         record.line);

                if (record.backTraceSize > 0) {
                    LOG_INFO("Stack trace:\n{}",
                             GetStackTraceString(record.backTrace,
                                                 record.backTraceSize));
                }
            }
        }

        void MemoryDebugger::DumpLeakReport() const {
            std::lock_guard<std::mutex> lock(mutex);

            if (allocations.empty()) {
                LOG_INFO("No memory leaks detected");
                return;
            }

            LOG_WARNING("Memory Leak Report:");
            LOG_WARNING("===================");

            size_t totalLeaked = 0;
            for (const auto &pair : allocations) {
                const AllocationRecord &record = pair.second;
                totalLeaked += record.size;

                LOG_WARNING(
                    "Leak: {} bytes at {}", record.size, record.address);
                LOG_WARNING("Type: {}",
                            record.typeName ? record.typeName : "Unknown");
                LOG_WARNING("Allocated at {}:{}",
                            record.location.file,
                            record.location.line);

                if (record.backTraceSize > 0) {
                    LOG_WARNING("Stack trace:\n{}",
                                GetStackTraceString(record.backTrace,
                                                    record.backTraceSize));
                }
                LOG_WARNING("------------------");
            }

            LOG_WARNING("Total leaked memory: {} bytes in {} allocations",
                        totalLeaked,
                        allocations.size());
        }

        void MemoryDebugger::DumpMemoryStats() const {
            std::lock_guard<std::mutex> lock(mutex);

            size_t totalAllocated = 0;
            size_t maxSize = 0;
            size_t minSize = SIZE_MAX;
            size_t totalGuardBandSize = 0;

            for (const auto &pair : allocations) {
                const AllocationRecord &record = pair.second;
                totalAllocated += record.size;
                maxSize = std::max(maxSize, record.size);
                minSize = std::min(minSize, record.size);
                totalGuardBandSize += config.guardSize * 2;
            }

            LOG_INFO("Memory Statistics:");
            LOG_INFO("=================");
            LOG_INFO("Active allocations: {}", allocations.size());
            LOG_INFO("Total allocated: {} bytes", totalAllocated);
            LOG_INFO(
                "Average allocation size: {} bytes",
                allocations.empty() ? 0 : totalAllocated / allocations.size());
            LOG_INFO("Smallest allocation: {} bytes",
                     minSize == SIZE_MAX ? 0 : minSize);
            LOG_INFO("Largest allocation: {} bytes", maxSize);
            LOG_INFO("Guard band overhead: {} bytes", totalGuardBandSize);
        }

        void MemoryDebugger::ClearAccessHistory() {
            std::lock_guard<std::mutex> lock(mutex);
            accessHistory.clear();
        }

        bool MemoryDebugger::CheckPattern(void *ptr,
                                          size_t size,
                                          uint8 pattern) const {
            const uint8 *bytes = static_cast<uint8 *>(ptr);
            for (size_t i = 0; i < size; ++i) {
                if (bytes[i] != pattern) {
                    return false;
                }
            }
            return true;
        }

        void MemoryDebugger::FillPattern(void *ptr,
                                         size_t size,
                                         uint8 pattern) {
            std::memset(ptr, pattern, size);
        }

        void MemoryDebugger::ValidateGuardBands(
            const AllocationRecord &record) const {
            if (!config.enableBoundaryChecking) return;

            // Check front guard band
            const unsigned char *frontGuardPtr =
                static_cast<const unsigned char *>(record.address) -
                config.guardSize;

            // Check back guard band
            const unsigned char *backGuardPtr =
                static_cast<const unsigned char *>(record.address) +
                record.size;

            if (!CheckPattern(const_cast<void *>(
                                  static_cast<const void *>(frontGuardPtr)),
                              config.guardSize,
                              GuardPattern)) {
                LOG_ERROR("Front guard band corrupted at {}", record.address);
                LOG_ERROR("Allocation info: {} bytes, type: {}",
                          record.size,
                          record.typeName ? record.typeName : "Unknown");
                LOG_ERROR("Location: {}:{}",
                          record.location.file,
                          record.location.line);
            }

            if (!CheckPattern(
                    const_cast<void *>(static_cast<const void *>(backGuardPtr)),
                    config.guardSize,
                    GuardPattern)) {
                LOG_ERROR("Back guard band corrupted at {}", record.address);
                LOG_ERROR("Allocation info: {} bytes, type: {}",
                          record.size,
                          record.typeName ? record.typeName : "Unknown");
                LOG_ERROR("Location: {}:{}",
                          record.location.file,
                          record.location.line);
            }
        }

        void *MemoryDebugger::PlaceGuardBands(void *ptr, size_t size) {
            if (!config.enableBoundaryChecking) return ptr;

            // Place front guard band
            uint8_t *guardPtr = static_cast<uint8_t *>(ptr);
            if (guardPtr) {
                FillPattern(guardPtr, config.guardSize, GuardPattern);

                // Calculate user memory location
                uint8_t *userPtr = guardPtr + config.guardSize;

                // Place back guard band
                uint8_t *backGuardPtr = userPtr + size;
                FillPattern(backGuardPtr, config.guardSize, GuardPattern);

                return userPtr;
            }
            return nullptr;
        }

        void MemoryDebugger::CaptureStackTrace(void **trace,
                                               size_t &depth) const {
#if PLATFORM_WINDOWS
            depth = CaptureStackBackTrace(
                1, std::min(32u, config.maxStackTrace), trace, nullptr);
#else
            depth = backtrace(trace, std::min(32u, config.maxStackTrace));
#endif
        }

        std::string MemoryDebugger::GetStackTraceString(void *const *trace,
                                                        size_t depth) const {
            std::string result;

#if PLATFORM_WINDOWS
            SYMBOL_INFO *symbol = (SYMBOL_INFO *)calloc(
                sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
            symbol->MaxNameLen = 255;
            symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

            for (size_t i = 0; i < depth; i++) {
                if (SymFromAddr(
                        GetCurrentProcess(), (DWORD64)trace[i], 0, symbol)) {
                    result += std::to_string(i) + ": " + symbol->Name + "\n";
                }
            }
            free(symbol);
#else
            char **symbols = backtrace_symbols(trace, depth);
            if (symbols) {
                for (size_t i = 0; i < depth; i++) {
                    result += std::to_string(i) + ": " + symbols[i] + "\n";
                }
                free(symbols);
            }
#endif

            return result;
        }

    }  // namespace Memory
}  // namespace Engine
