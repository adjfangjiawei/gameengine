
#pragma once

#include <string>
#include <unordered_map>

#include "CoreTypes.h"
#include "Error/ErrorSystem.h"

namespace Engine {

    // Error code type definition
    using ErrorCode = uint32;

    // Error code ranges for different subsystems
    // Each subsystem has a range of 1000 error codes
    enum ErrorCodeRanges {
        ERROR_SUCCESS = 0,
        ERROR_GENERAL_BEGIN = 1,
        ERROR_SYSTEM_BEGIN = 1000,
        ERROR_MEMORY_BEGIN = 2000,
        ERROR_FILE_BEGIN = 3000,
        ERROR_NETWORK_BEGIN = 4000,
        ERROR_RENDER_BEGIN = 5000,
        ERROR_AUDIO_BEGIN = 6000,
        ERROR_INPUT_BEGIN = 7000,
        ERROR_PHYSICS_BEGIN = 8000,
        ERROR_SCRIPT_BEGIN = 9000
    };

    // General errors (1-999)
    enum GeneralErrors {
        ERROR_UNKNOWN = ERROR_GENERAL_BEGIN + 1,
        ERROR_INVALID_ARGUMENT = ERROR_GENERAL_BEGIN + 2,
        ERROR_INVALID_OPERATION = ERROR_GENERAL_BEGIN + 3,
        ERROR_NOT_IMPLEMENTED = ERROR_GENERAL_BEGIN + 4,
        ERROR_NOT_INITIALIZED = ERROR_GENERAL_BEGIN + 5,
        ERROR_ALREADY_EXISTS = ERROR_GENERAL_BEGIN + 6,
        ERROR_NOT_FOUND = ERROR_GENERAL_BEGIN + 7,
        ERROR_TIMEOUT = ERROR_GENERAL_BEGIN + 8,
        ERROR_OVERFLOW = ERROR_GENERAL_BEGIN + 9,
        ERROR_UNDERFLOW = ERROR_GENERAL_BEGIN + 10
    };

    // System errors (1000-1999)
    enum SystemErrors {
        ERROR_SYSTEM_CALL = ERROR_SYSTEM_BEGIN + 1,
        ERROR_SYSTEM_RESOURCE = ERROR_SYSTEM_BEGIN + 2,
        ERROR_SYSTEM_PERMISSION = ERROR_SYSTEM_BEGIN + 3
    };

    // Memory errors (2000-2999)
    enum MemoryErrors {
        ERROR_OUT_OF_MEMORY = ERROR_MEMORY_BEGIN + 1,
        ERROR_MEMORY_CORRUPT = ERROR_MEMORY_BEGIN + 2,
        ERROR_INVALID_POINTER = ERROR_MEMORY_BEGIN + 3,
        ERROR_BUFFER_OVERFLOW = ERROR_MEMORY_BEGIN + 4
    };

    // File errors (3000-3999)
    enum FileErrors {
        ERROR_FILE_NOT_FOUND = ERROR_FILE_BEGIN + 1,
        ERROR_FILE_ACCESS = ERROR_FILE_BEGIN + 2,
        ERROR_FILE_READ = ERROR_FILE_BEGIN + 3,
        ERROR_FILE_WRITE = ERROR_FILE_BEGIN + 4,
        ERROR_FILE_EOF = ERROR_FILE_BEGIN + 5
    };

    class ErrorCodeManager {
      public:
        static ErrorCodeManager& Get();

        // Get error message for a specific error code
        std::string GetErrorMessage(ErrorCode code) const;

        // Register custom error code and message
        void RegisterError(ErrorCode code, const std::string& message);

        // Check if an error code is registered
        bool IsErrorRegistered(ErrorCode code) const;

        // Get error category from error code
        ErrorCategory GetErrorCategory(ErrorCode code) const;

      private:
        ErrorCodeManager();
        ~ErrorCodeManager() = default;

        ErrorCodeManager(const ErrorCodeManager&) = delete;
        ErrorCodeManager& operator=(const ErrorCodeManager&) = delete;

        void InitializeDefaultErrors();

        std::unordered_map<ErrorCode, std::string> m_errorMessages;
    };

    // Convenience function to get error message
    inline std::string GetErrorMessage(ErrorCode code) {
        return ErrorCodeManager::Get().GetErrorMessage(code);
    }

}  // namespace Engine
