
#include "Core/Assert.h"

#include <atomic>
#include <fstream>
#include <mutex>
#include <string>

namespace Engine {

    namespace {
        AssertHandler g_AssertHandler = Detail::DefaultAssertHandler;
        std::mutex g_AssertHandlerMutex;
    }  // namespace

    AssertHandler GetAssertHandler() {
        std::lock_guard<std::mutex> lock(g_AssertHandlerMutex);
        return g_AssertHandler;
    }

    void SetAssertHandler(AssertHandler handler) {
        std::lock_guard<std::mutex> lock(g_AssertHandlerMutex);
        g_AssertHandler = handler;
    }

    bool IsDebuggerPresentPlatformSpecific() {
#ifdef _WIN32
        return IsDebuggerPresent();
#else
        std::ifstream statusFile("/proc/self/status");
        if (statusFile.is_open()) {
            std::string line;
            while (std::getline(statusFile, line)) {
                if (line.substr(0, 11) == "TracerPid:\t") {
                    return line.substr(11) != "0";
                }
            }
        }
        return false;
#endif
    }

    namespace Detail {

        std::string FormatAssertMessage(const char *condition,
                                        const char *message,
                                        const char *file,
                                        int line) {
            std::string result;
            result.reserve(256);  // Reserve space to avoid reallocations

            result += "Assertion '";
            result += condition;
            result += "' failed";

            if (message && message[0] != '\0') {
                result += ": ";
                result += message;
            }

            result += " (";
            result += file;
            result += ":";
            result += std::to_string(line);
            result += ")";

            return result;
        }

        bool DefaultAssertHandler(const char *condition,
                                  const char *message,
                                  const char *file,
                                  int line,
                                  AssertType type) {
            std::string assertMessage =
                FormatAssertMessage(condition, message, file, line);

            switch (type) {
                case AssertType::Warning:
                    LOG_WARNING("{}", assertMessage);
                    return true;

                case AssertType::Error:
                    LOG_ERROR("{}", assertMessage);
                    return IsDebuggerPresentPlatformSpecific();

                case AssertType::Fatal:
                    LOG_FATAL("{}", assertMessage);
                    std::abort();
                    return false;  // Never reached
            }

            return false;  // Never reached
        }

    }  // namespace Detail

}  // namespace Engine
