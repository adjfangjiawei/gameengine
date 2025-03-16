
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>

namespace Engine {

    // Integer types
    using int8 = std::int8_t;
    using int16 = std::int16_t;
    using int32 = std::int32_t;
    using int64 = std::int64_t;
    using uint8 = std::uint8_t;
    using uint16 = std::uint16_t;
    using uint32 = std::uint32_t;
    using uint64 = std::uint64_t;

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
#define PLATFORM_WINDOWS 1
#elif defined(__linux__) || defined(__unix__)
#define PLATFORM_LINUX 1
#else
#error "Unsupported platform"
#endif

// Architecture detection
#if defined(_M_X64) || defined(__x86_64__)
#define ARCH_X64 1
#elif defined(_M_IX86) || defined(__i386__)
#define ARCH_X86 1
#else
#error "Unsupported architecture"
#endif

// Compiler detection
#if defined(_MSC_VER)
#define COMPILER_MSVC 1
#elif defined(__clang__)
#define COMPILER_CLANG 1
#elif defined(__GNUC__)
#define COMPILER_GCC 1
#else
#error "Unsupported compiler"
#endif

// Build configuration - only define if not already defined
#ifndef BUILD_DEBUG
#if defined(_DEBUG) || defined(DEBUG)
#define BUILD_DEBUG 1
#define BUILD_RELEASE 0
#elif defined(NDEBUG) || defined(RELEASE)
#define BUILD_DEBUG 0
#define BUILD_RELEASE 1
#else
#define BUILD_DEBUG 0
#define BUILD_RELEASE 1  // Default to release if not specified
#endif
#endif

#ifndef BUILD_RELEASE
#define BUILD_RELEASE !BUILD_DEBUG
#endif

// Byte order detection
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define PLATFORM_BIG_ENDIAN 1
#else
#define PLATFORM_LITTLE_ENDIAN 1
#endif

// Common macros
#define UNUSED(x) (void)(x)
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#define ALIGN_UP(value, alignment) ((value + alignment - 1) & ~(alignment - 1))
#define IS_POWER_OF_TWO(x) (x && !(x & (x - 1)))

    // Bit operations
    template <typename T>
    inline T AlignUp(T value, size_t alignment) {
        return static_cast<T>(ALIGN_UP(static_cast<size_t>(value), alignment));
    }

    template <typename T>
    inline bool IsPowerOfTwo(T value) {
        return value && !(value & (value - 1));
    }

    template <typename T>
    inline T SetBit(T value, uint32 bit) {
        return value | (T(1) << bit);
    }

    template <typename T>
    inline T ClearBit(T value, uint32 bit) {
        return value & ~(T(1) << bit);
    }

    template <typename T>
    inline bool IsBitSet(T value, uint32 bit) {
        return (value & (T(1) << bit)) != 0;
    }

    // Type traits helpers
    template <typename T>
    using RemoveRef = typename std::remove_reference<T>::type;

    template <typename T>
    using RemovePtr = typename std::remove_pointer<T>::type;

    template <typename T>
    using RemoveCV = typename std::remove_cv<T>::type;

    template <typename T>
    using RemoveAll = RemoveCV<RemoveRef<RemovePtr<T>>>;

}  // namespace Engine
