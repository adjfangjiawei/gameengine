
#include "Core/Error/ErrorCodes.h"

namespace Engine {

    ErrorCodeManager::ErrorCodeManager() { InitializeDefaultErrors(); }

    ErrorCodeManager& ErrorCodeManager::Get() {
        static ErrorCodeManager instance;
        return instance;
    }

    void ErrorCodeManager::InitializeDefaultErrors() {
        // General errors
        m_errorMessages[ERROR_SUCCESS] = "Operation completed successfully";
        m_errorMessages[ERROR_UNKNOWN] = "Unknown error occurred";
        m_errorMessages[ERROR_INVALID_ARGUMENT] = "Invalid argument provided";
        m_errorMessages[ERROR_INVALID_OPERATION] =
            "Invalid operation attempted";
        m_errorMessages[ERROR_NOT_IMPLEMENTED] = "Feature not implemented";
        m_errorMessages[ERROR_NOT_INITIALIZED] = "Component not initialized";
        m_errorMessages[ERROR_ALREADY_EXISTS] = "Item already exists";
        m_errorMessages[ERROR_NOT_FOUND] = "Item not found";
        m_errorMessages[ERROR_TIMEOUT] = "Operation timed out";
        m_errorMessages[ERROR_OVERFLOW] = "Overflow occurred";
        m_errorMessages[ERROR_UNDERFLOW] = "Underflow occurred";

        // System errors
        m_errorMessages[ERROR_SYSTEM_CALL] = "System call failed";
        m_errorMessages[ERROR_SYSTEM_RESOURCE] = "System resource unavailable";
        m_errorMessages[ERROR_SYSTEM_PERMISSION] = "Permission denied";

        // Memory errors
        m_errorMessages[ERROR_OUT_OF_MEMORY] = "Out of memory";
        m_errorMessages[ERROR_MEMORY_CORRUPT] = "Memory corruption detected";
        m_errorMessages[ERROR_INVALID_POINTER] = "Invalid pointer operation";
        m_errorMessages[ERROR_BUFFER_OVERFLOW] = "Buffer overflow detected";

        // File errors
        m_errorMessages[ERROR_FILE_NOT_FOUND] = "File not found";
        m_errorMessages[ERROR_FILE_ACCESS] = "File access denied";
        m_errorMessages[ERROR_FILE_READ] = "File read error";
        m_errorMessages[ERROR_FILE_WRITE] = "File write error";
        m_errorMessages[ERROR_FILE_EOF] = "End of file reached";
    }

    std::string ErrorCodeManager::GetErrorMessage(ErrorCode code) const {
        auto it = m_errorMessages.find(code);
        if (it != m_errorMessages.end()) {
            return it->second;
        }
        return "Unknown error code: " + std::to_string(code);
    }

    void ErrorCodeManager::RegisterError(ErrorCode code,
                                         const std::string& message) {
        m_errorMessages[code] = message;
    }

    bool ErrorCodeManager::IsErrorRegistered(ErrorCode code) const {
        return m_errorMessages.find(code) != m_errorMessages.end();
    }

    ErrorCategory ErrorCodeManager::GetErrorCategory(ErrorCode code) const {
        if (code >= ERROR_GENERAL_BEGIN && code < ERROR_SYSTEM_BEGIN) {
            return ErrorCategory::System;
        } else if (code >= ERROR_SYSTEM_BEGIN && code < ERROR_MEMORY_BEGIN) {
            return ErrorCategory::System;
        } else if (code >= ERROR_MEMORY_BEGIN && code < ERROR_FILE_BEGIN) {
            return ErrorCategory::Memory;
        } else if (code >= ERROR_FILE_BEGIN && code < ERROR_NETWORK_BEGIN) {
            return ErrorCategory::File;
        } else if (code >= ERROR_NETWORK_BEGIN && code < ERROR_RENDER_BEGIN) {
            return ErrorCategory::Network;
        } else if (code >= ERROR_RENDER_BEGIN && code < ERROR_AUDIO_BEGIN) {
            return ErrorCategory::Render;
        } else if (code >= ERROR_AUDIO_BEGIN && code < ERROR_INPUT_BEGIN) {
            return ErrorCategory::Audio;
        } else if (code >= ERROR_INPUT_BEGIN && code < ERROR_PHYSICS_BEGIN) {
            return ErrorCategory::Input;
        } else if (code >= ERROR_PHYSICS_BEGIN && code < ERROR_SCRIPT_BEGIN) {
            return ErrorCategory::Physics;
        } else if (code >= ERROR_SCRIPT_BEGIN) {
            return ErrorCategory::Script;
        }
        return ErrorCategory::None;
    }

}  // namespace Engine
