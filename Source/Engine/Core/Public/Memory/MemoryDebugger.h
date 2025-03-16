
#pragma once

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "Memory/MemoryTypes.h"
#include "CoreTypes.h"

namespace Engine {
    namespace Memory {

        class MemoryDebugger {
          public:
            struct Config {
                bool enableBoundaryChecking{
                    true};  // Enable memory boundary checking
                bool enableAccessTracking{
                    false};  // Enable memory access tracking
                bool enableLeakDetection{true};  // Enable memory leak detection
                bool enablePatternFill{true};    // Fill memory with patterns
                size_t guardSize{8};             // Size of guard bands in bytes
                uint32 maxStackTrace{32};  // Maximum depth of stack traces
                size_t maxAccessHistory{
                    1000};  // Maximum number of access records to keep
            };

            struct AccessRecord {
                void *address;         // Memory address accessed
                size_t size;           // Size of access
                bool isWrite;          // true for write, false for read
                uint64 timestamp;      // Time of access
                const char *file;      // Source file
                int line;              // Line number
                void *backTrace[32];   // Stack trace
                size_t backTraceSize;  // Stack trace depth
            };

            struct AllocationRecord {
                void *address;            // Allocated memory address
                size_t size;              // Allocation size
                size_t alignment;         // Alignment requirement
                const char *typeName;     // Type name if available
                MemoryLocation location;  // Source location
                uint64 timestamp;         // Allocation time
                void *backTrace[32];      // Stack trace
                size_t backTraceSize;     // Stack trace depth
            };

            static MemoryDebugger &Get();

            void Initialize(const Config &config);
            void Shutdown();

            // Memory operation hooks
            void *OnAllocation(void *ptr,
                               size_t size,
                               size_t alignment,
                               const char *typeName,
                               const MemoryLocation &location);
            void OnDeallocation(void *ptr, const MemoryLocation &location);
            void OnAccess(void *ptr,
                          size_t size,
                          bool isWrite,
                          const MemoryLocation &location);

            // Debug functions
            void ValidateAllAllocations();
            void DumpAccessHistory() const;
            void DumpLeakReport() const;
            void DumpMemoryStats() const;
            void ClearAccessHistory();

            // Pattern checking
            bool CheckPattern(void *ptr, size_t size, uint8 pattern) const;
            void FillPattern(void *ptr, size_t size, uint8 pattern);

          private:
            MemoryDebugger() = default;
            ~MemoryDebugger() = default;
            MemoryDebugger(const MemoryDebugger &) = delete;
            MemoryDebugger &operator=(const MemoryDebugger &) = delete;

            struct GuardBand {
                static constexpr uint8 Pattern = 0xFD;
                uint8 data[1];  // Actual size determined by config
            };

            void ValidateGuardBands(const AllocationRecord &record) const;
            void *PlaceGuardBands(void *ptr, size_t size);
            void CaptureStackTrace(void **trace, size_t &depth) const;
            std::string GetStackTraceString(void *const *trace,
                                            size_t depth) const;

            Config config;
            std::unordered_map<void *, AllocationRecord> allocations;
            std::vector<AccessRecord> accessHistory;
            mutable std::mutex mutex;

            // Memory patterns
            static constexpr uint8 NewPattern =
                0xCD;  // Pattern for new allocations
            static constexpr uint8 DeletePattern =
                0xDD;  // Pattern after deletion
            static constexpr uint8 GuardPattern =
                0xFD;  // Pattern for guard bands
        };

// Helper macros for memory debugging
#if BUILD_DEBUG
#define TRACK_MEMORY_ACCESS(ptr, size, isWrite)     \
    Engine::Memory::MemoryDebugger::Get().OnAccess( \
        ptr, size, isWrite, MEMORY_LOCATION)
#define VALIDATE_MEMORY() \
    Engine::Memory::MemoryDebugger::Get().ValidateAllAllocations()
#else
#define TRACK_MEMORY_ACCESS(ptr, size, isWrite) ((void)0)
#define VALIDATE_MEMORY() ((void)0)
#endif

    }  // namespace Memory
}  // namespace Engine
