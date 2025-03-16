
#pragma once

#include "Core/Log/LogSystem.h"

namespace Engine {

    enum class AssertType {
        Warning,  // Non-fatal warning
        Error,    // Non-fatal error
        Fatal     // Fatal error
    };

    // Function pointer type for assert handlers
    using AssertHandler = bool (*)(const char *condition,
                                   const char *message,
                                   const char *file,
                                   int line,
                                   AssertType type);

    // Get current assert handler
    AssertHandler GetAssertHandler();

    // Set custom assert handler
    void SetAssertHandler(AssertHandler handler);

    // Check if debugger is present
    bool IsDebuggerPresentPlatformSpecific();

    namespace Detail {
        // Default assert handler
        bool DefaultAssertHandler(const char *condition,
                                  const char *message,
                                  const char *file,
                                  int line,
                                  AssertType type);

        // Format assert message
        std::string FormatAssertMessage(const char *condition,
                                        const char *message,
                                        const char *file,
                                        int line);
    }  // namespace Detail

#if BUILD_DEBUG
#define ASSERT(condition, str)                                     \
    do {                                                           \
        if (!(condition)) {                                        \
            LOG_ERROR("Assertion failed: {},{}", #condition, str); \
            std::abort();                                          \
        }                                                          \
    } while (0)

#define ASSERT_MSG(condition, message, type)                             \
    do {                                                                 \
        if (!(condition)) {                                              \
            LOG_ERROR("Assertion failed: {} - {}", #condition, message); \
            if (type == Engine::AssertType::Fatal) {                     \
                std::abort();                                            \
            }                                                            \
        }                                                                \
    } while (0)

#define ASSERT_SOFT(condition, message) \
    ASSERT_MSG(condition, message, Engine::AssertType::Warning)

#define VERIFY(condition)                                     \
    do {                                                      \
        if (!(condition)) {                                   \
            LOG_ERROR("Verification failed: {}", #condition); \
        }                                                     \
    } while (0)
#else
#define ASSERT(condition, str) ((void)0)
#define ASSERT_MSG(condition, message, type) ((void)0)
#define ASSERT_SOFT(condition, message) ((void)0)
#define VERIFY(condition) (condition)
#endif

}  // namespace Engine
