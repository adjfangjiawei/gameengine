
#pragma once

#include <string>

#include "CoreTypes.h"

namespace Engine {
    // File permission flags
    enum class FilePermission {
        None = 0,
        UserRead = 1 << 0,
        UserWrite = 1 << 1,
        UserExecute = 1 << 2,
        GroupRead = 1 << 3,
        GroupWrite = 1 << 4,
        GroupExecute = 1 << 5,
        OtherRead = 1 << 6,
        OtherWrite = 1 << 7,
        OtherExecute = 1 << 8,

        // Common combinations
        UserAll = UserRead | UserWrite | UserExecute,
        GroupAll = GroupRead | GroupWrite | GroupExecute,
        OtherAll = OtherRead | OtherWrite | OtherExecute,

        // Default permissions
        DefaultFile = UserRead | UserWrite | GroupRead | OtherRead,
        DefaultDir =
            UserAll | GroupRead | GroupExecute | OtherRead | OtherExecute
    };

    inline FilePermission operator|(FilePermission a, FilePermission b) {
        return static_cast<FilePermission>(static_cast<int>(a) |
                                           static_cast<int>(b));
    }

    inline FilePermission operator&(FilePermission a, FilePermission b) {
        return static_cast<FilePermission>(static_cast<int>(a) &
                                           static_cast<int>(b));
    }

    // File ownership
    struct FileOwner {
        uint32 userId;
        uint32 groupId;
        std::string userName;
        std::string groupName;
    };

    class FilePermissions {
      public:
        // Get/Set permissions
        static bool SetPermissions(const std::string &path,
                                   FilePermission permissions);
        static FilePermission GetPermissions(const std::string &path);

        // Get/Set ownership
        static bool SetOwner(const std::string &path,
                             uint32 userId,
                             uint32 groupId);
        static bool SetOwner(const std::string &path,
                             const std::string &userName,
                             const std::string &groupName);
        static FileOwner GetOwner(const std::string &path);

        // Check permissions
        static bool CanRead(const std::string &path);
        static bool CanWrite(const std::string &path);
        static bool CanExecute(const std::string &path);

        // Convert between platform-specific and FilePermission
#if PLATFORM_WINDOWS
        static FilePermission FromWindowsAttributes(uint32 attributes);
        static uint32 ToWindowsAttributes(FilePermission permissions);
#else
        static FilePermission FromPosixMode(mode_t mode);
        static mode_t ToPosixMode(FilePermission permissions);
#endif

      private:
        static bool CheckAccess(const std::string &path,
                                FilePermission permission);
    };

}  // namespace Engine
