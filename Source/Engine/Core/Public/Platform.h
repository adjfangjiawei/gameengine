
#pragma once

#include <string>

#include "CoreTypes.h"

#if PLATFORM_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <limits.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace Engine {
    namespace Platform {

// Platform-specific types
#if PLATFORM_WINDOWS
        using FileHandle = HANDLE;
        const FileHandle InvalidFileHandle = INVALID_HANDLE_VALUE;
#define PATH_MAX_LENGTH MAX_PATH
#else
        using FileHandle = int;
        const FileHandle InvalidFileHandle = -1;
#define PATH_MAX_LENGTH PATH_MAX
#endif

// Path separators
#if PLATFORM_WINDOWS
        constexpr char PathSeparator = '\\';
        constexpr const char *PathSeparatorStr = "\\";
        constexpr const char *LineEnd = "\r\n";
#else
        constexpr char PathSeparator = '/';
        constexpr const char *PathSeparatorStr = "/";
        constexpr const char *LineEnd = "\n";
#endif

// Shared library extensions
#if PLATFORM_WINDOWS
        constexpr const char *SharedLibraryPrefix = "";
        constexpr const char *SharedLibrarySuffix = ".dll";
#elif PLATFORM_MAC
        constexpr const char *SharedLibraryPrefix = "lib";
        constexpr const char *SharedLibrarySuffix = ".dylib";
#else
        constexpr const char *SharedLibraryPrefix = "lib";
        constexpr const char *SharedLibrarySuffix = ".so";
#endif

        // Debug functions
        bool IsDebuggerPresent();
        void DebugBreak();

        // Platform information
        const char *GetPlatformName();
        const char *GetArchitectureName();
        const char *GetCompilerName();

        // System properties
        bool IsLittleEndian();
        size_t GetPageSize();
        size_t GetAllocationGranularity();

        // Memory functions
        void *AlignedAlloc(size_t size, size_t alignment);
        void AlignedFree(void *ptr);

        // Process and thread functions
        uint32 GetCurrentProcessId();
        uint32 GetCurrentThreadId();

        // Environment functions
        bool SetEnvironmentVariable(const char *name, const char *value);
        std::string GetEnvironmentVariable(const char *name);

        // Dynamic library functions
        void *LoadDynamicLibrary(const char *path);
        void UnloadDynamicLibrary(void *handle);
        void *GetDynamicLibrarySymbol(void *handle, const char *name);
        std::string GetDynamicLibraryError();

        // Memory alignment helpers
        template <typename T>
        constexpr bool IsAligned(T value, size_t alignment) {
            return (reinterpret_cast<uintptr_t>(value) & (alignment - 1)) == 0;
        }

        template <typename T>
        constexpr T AlignUp(T value, size_t alignment) {
            return reinterpret_cast<T>(
                (reinterpret_cast<uintptr_t>(value) + (alignment - 1)) &
                ~(alignment - 1));
        }

        template <typename T>
        constexpr T AlignDown(T value, size_t alignment) {
            return reinterpret_cast<T>(reinterpret_cast<uintptr_t>(value) &
                                       ~(alignment - 1));
        }

        // Byte order conversion
        inline uint16 ByteSwap16(uint16 value) {
            return (value << 8) | (value >> 8);
        }

        inline uint32 ByteSwap32(uint32 value) {
            return (value << 24) | ((value << 8) & 0x00FF0000) |
                   ((value >> 8) & 0x0000FF00) | (value >> 24);
        }

        inline uint64 ByteSwap64(uint64 value) {
            return (value << 56) | ((value << 40) & 0x00FF000000000000ULL) |
                   ((value << 24) & 0x0000FF0000000000ULL) |
                   ((value << 8) & 0x000000FF00000000ULL) |
                   ((value >> 8) & 0x00000000FF000000ULL) |
                   ((value >> 24) & 0x0000000000FF0000ULL) |
                   ((value >> 40) & 0x000000000000FF00ULL) | (value >> 56);
        }

// Endian conversion macros
#if PLATFORM_LITTLE_ENDIAN
#define PLATFORM_LTOH16(x) (x)
#define PLATFORM_LTOH32(x) (x)
#define PLATFORM_LTOH64(x) (x)
#define PLATFORM_HTOL16(x) (x)
#define PLATFORM_HTOL32(x) (x)
#define PLATFORM_HTOL64(x) (x)
#define PLATFORM_BTOH16(x) ByteSwap16(x)
#define PLATFORM_BTOH32(x) ByteSwap32(x)
#define PLATFORM_BTOH64(x) ByteSwap64(x)
#define PLATFORM_HTOB16(x) ByteSwap16(x)
#define PLATFORM_HTOB32(x) ByteSwap32(x)
#define PLATFORM_HTOB64(x) ByteSwap64(x)
#else
#define PLATFORM_LTOH16(x) ByteSwap16(x)
#define PLATFORM_LTOH32(x) ByteSwap32(x)
#define PLATFORM_LTOH64(x) ByteSwap64(x)
#define PLATFORM_HTOL16(x) ByteSwap16(x)
#define PLATFORM_HTOL32(x) ByteSwap32(x)
#define PLATFORM_HTOL64(x) ByteSwap64(x)
#define PLATFORM_BTOH16(x) (x)
#define PLATFORM_BTOH32(x) (x)
#define PLATFORM_BTOB64(x) (x)
#define PLATFORM_HTOB16(x) (x)
#define PLATFORM_HTOB32(x) (x)
#define PLATFORM_HTOB64(x) (x)
#endif

// Platform-specific macros
#if PLATFORM_WINDOWS
#define PLATFORM_BREAK() __debugbreak()
#define PLATFORM_FUNCTION __FUNCTION__
#define PLATFORM_SHARED_EXPORT __declspec(dllexport)
#define PLATFORM_SHARED_IMPORT __declspec(dllimport)
#else
#define PLATFORM_BREAK() raise(SIGTRAP)
#define PLATFORM_FUNCTION __func__
#define PLATFORM_SHARED_EXPORT __attribute__((visibility("default")))
#define PLATFORM_SHARED_IMPORT
#endif

    }  // namespace Platform
}  // namespace Engine
