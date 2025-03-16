
#include "Core/Platform.h"

#include <cctype>
#include <cstring>

#include "Core/Assert.h"

#if PLATFORM_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#include <fstream>
#endif

namespace Engine {
    namespace Platform {

#if PLATFORM_WINDOWS
        bool IsDebuggerPresent() { return ::IsDebuggerPresent() != 0; }

        void DebugBreak() { ::DebugBreak(); }

        uint32 GetCurrentThreadId() {
            return static_cast<uint32>(::GetCurrentThreadId());
        }

        uint32 GetCurrentProcessId() {
            return static_cast<uint32>(::GetCurrentProcessId());
        }

#else
        bool IsDebuggerPresent() {
            // Check if process is being traced
            std::ifstream statusFile("/proc/self/status");
            if (!statusFile.is_open()) {
                return false;
            }

            std::string line;
            while (std::getline(statusFile, line)) {
                if (line.substr(0, 10) == "TracerPid:") {
                    return std::stoi(line.substr(10)) != 0;
                }
            }
            return false;
        }

        void DebugBreak() { raise(SIGTRAP); }

        uint32 GetCurrentThreadId() {
#if PLATFORM_MAC
            uint64_t tid;
            pthread_threadid_np(nullptr, &tid);
            return static_cast<uint32>(tid);
#else
            // On Linux, use syscall to get thread ID
            pid_t tid = syscall(SYS_gettid);
            return static_cast<uint32>(tid);
#endif
        }

        uint32 GetCurrentProcessId() { return static_cast<uint32>(getpid()); }
#endif

        const char *GetPlatformName() {
#if PLATFORM_WINDOWS
            return "Windows";
#elif PLATFORM_LINUX
            return "Linux";
#elif PLATFORM_MAC
            return "MacOS";
#else
            return "Unknown";
#endif
        }

        const char *GetArchitectureName() {
#if ARCH_X64
            return "x64";
#elif ARCH_X86
            return "x86";
#elif ARCH_ARM64
            return "ARM64";
#elif ARCH_ARM32
            return "ARM32";
#else
            return "Unknown";
#endif
        }

        const char *GetCompilerName() {
#if COMPILER_MSVC
            return "MSVC";
#elif COMPILER_CLANG
            return "Clang";
#elif COMPILER_GCC
            return "GCC";
#else
            return "Unknown";
#endif
        }

        bool IsLittleEndian() {
            static const uint16_t value = 0x0001;
            return *reinterpret_cast<const uint8_t *>(&value) == 0x01;
        }

        size_t GetPageSize() {
#if PLATFORM_WINDOWS
            SYSTEM_INFO sysInfo;
            GetSystemInfo(&sysInfo);
            return sysInfo.dwPageSize;
#else
            return static_cast<size_t>(sysconf(_SC_PAGESIZE));
#endif
        }

        size_t GetAllocationGranularity() {
#if PLATFORM_WINDOWS
            SYSTEM_INFO sysInfo;
            GetSystemInfo(&sysInfo);
            return sysInfo.dwAllocationGranularity;
#else
            return GetPageSize();
#endif
        }

        void *AlignedAlloc(size_t size, size_t alignment) {
#if PLATFORM_WINDOWS
            return _aligned_malloc(size, alignment);
#else
            void *ptr = nullptr;
            if (posix_memalign(&ptr, alignment, size) != 0) {
                return nullptr;
            }
            return ptr;
#endif
        }

        void AlignedFree(void *ptr) {
#if PLATFORM_WINDOWS
            _aligned_free(ptr);
#else
            free(ptr);
#endif
        }

        bool SetEnvironmentVariable(const char *name, const char *value) {
#if PLATFORM_WINDOWS
            return SetEnvironmentVariableA(name, value) != 0;
#else
            return setenv(name, value, 1) == 0;
#endif
        }

        std::string GetEnvironmentVariable(const char *name) {
#if PLATFORM_WINDOWS
            char buffer[32767];  // Maximum size for Windows
            DWORD size = GetEnvironmentVariableA(name, buffer, sizeof(buffer));
            return size > 0 ? std::string(buffer, size) : std::string();
#else
            const char *value = getenv(name);
            return value ? std::string(value) : std::string();
#endif
        }

        void *LoadDynamicLibrary(const char *path) {
#if PLATFORM_WINDOWS
            return LoadLibraryA(path);
#else
            return dlopen(path, RTLD_NOW);
#endif
        }

        void UnloadDynamicLibrary(void *handle) {
#if PLATFORM_WINDOWS
            FreeLibrary(static_cast<HMODULE>(handle));
#else
            dlclose(handle);
#endif
        }

        void *GetDynamicLibrarySymbol(void *handle, const char *name) {
#if PLATFORM_WINDOWS
            return GetProcAddress(static_cast<HMODULE>(handle), name);
#else
            return dlsym(handle, name);
#endif
        }

        std::string GetDynamicLibraryError() {
#if PLATFORM_WINDOWS
            DWORD error = GetLastError();
            if (error == 0) return "";

            char *messageBuffer = nullptr;
            size_t size = FormatMessageA(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr,
                error,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                reinterpret_cast<LPSTR>(&messageBuffer),
                0,
                nullptr);

            std::string message(messageBuffer, size);
            LocalFree(messageBuffer);
            return message;
#else
            const char *error = dlerror();
            return error ? std::string(error) : std::string();
#endif
        }

    }  // namespace Platform
}  // namespace Engine
