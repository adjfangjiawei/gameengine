// #include "Core/FileSystem/FilePermissions.h"

// #if PLATFORM_WINDOWS
// #include <windows.h>
// #else
// #include <grp.h>
// #include <pwd.h>
// #include <sys/stat.h>
// #endif

// namespace Engine {
//     namespace FileSystem {

//         bool FilePermissions::SetPermissions(const std::string& path,
//                                              FilePermission permissions) {
// #if PLATFORM_WINDOWS
//             DWORD attributes = ToWindowsAttributes(permissions);
//             return SetFileAttributesA(path.c_str(), attributes) != 0;
// #else
//             mode_t mode = ToPosixMode(permissions);
//             return chmod(path.c_str(), mode) == 0;
// #endif
//         }

//         FilePermission FilePermissions::GetPermissions(
//             const std::string& path) {
// #if PLATFORM_WINDOWS
//             DWORD attributes = GetFileAttributesA(path.c_str());
//             if (attributes == INVALID_FILE_ATTRIBUTES) {
//                 return FilePermission::None;
//             }
//             return FromWindowsAttributes(attributes);
// #else
//             struct stat st;
//             if (stat(path.c_str(), &st) != 0) {
//                 return FilePermission::None;
//             }
//             return FromPosixMode(st.st_mode);
// #endif
//         }

//         bool FilePermissions::SetOwner(const std::string& path,
//                                        uint32 userId,
//                                        uint32 groupId) {
// #if PLATFORM_WINDOWS
//             // Windows doesn't have a direct equivalent
//             return false;
// #else
//             return chown(path.c_str(), userId, groupId) == 0;
// #endif
//         }

//         bool FilePermissions::SetOwner(const std::string& path,
//                                        const std::string& userName,
//                                        const std::string& groupName) {
// #if PLATFORM_WINDOWS
//             // Windows doesn't have a direct equivalent
//             return false;
// #else
//             struct passwd* pwd = getpwnam(userName.c_str());
//             struct group* grp = getgrnam(groupName.c_str());

//             if (!pwd || !grp) {
//                 return false;
//             }

//             return chown(path.c_str(), pwd->pw_uid, grp->gr_gid) == 0;
// #endif
//         }

//         FileOwner FilePermissions::GetOwner(const std::string& path) {
//             FileOwner owner;

// #if PLATFORM_WINDOWS
//             // Windows implementation would be more complex and require
//             security
//             // APIs
//             owner.userId = 0;
//             owner.groupId = 0;
//             owner.userName = "Unknown";
//             owner.groupName = "Unknown";
// #else
//             struct stat st;
//             if (stat(path.c_str(), &st) == 0) {
//                 owner.userId = st.st_uid;
//                 owner.groupId = st.st_gid;

//                 struct passwd* pwd = getpwuid(st.st_uid);
//                 struct group* grp = getgrgid(st.st_gid);

//                 if (pwd) {
//                     owner.userName = pwd->pw_name;
//                 }

//                 if (grp) {
//                     owner.groupName = grp->gr_name;
//                 }
//             }
// #endif

//             return owner;
//         }

//         bool FilePermissions::CanRead(const std::string& path) {
//             return CheckAccess(path, FilePermission::UserRead);
//         }

//         bool FilePermissions::CanWrite(const std::string& path) {
//             return CheckAccess(path, FilePermission::UserWrite);
//         }

//         bool FilePermissions::CanExecute(const std::string& path) {
//             return CheckAccess(path, FilePermission::UserExecute);
//         }

// #if PLATFORM_WINDOWS
//         FilePermission FilePermissions::FromWindowsAttributes(
//             uint32 attributes) {
//             FilePermission permissions = FilePermission::None;

//             if ((attributes & FILE_ATTRIBUTE_READONLY) == 0) {
//                 permissions = permissions | FilePermission::UserWrite;
//             }

//             // Always add read permission if file exists
//             permissions = permissions | FilePermission::UserRead;

//             if (attributes & FILE_ATTRIBUTE_DIRECTORY) {
//                 permissions = permissions | FilePermission::UserExecute;
//             }

//             return permissions;
//         }

//         uint32 FilePermissions::ToWindowsAttributes(
//             FilePermission permissions) {
//             DWORD attributes = FILE_ATTRIBUTE_NORMAL;

//             if ((permissions & FilePermission::UserWrite) ==
//                 FilePermission::None) {
//                 attributes |= FILE_ATTRIBUTE_READONLY;
//             }

//             return attributes;
//         }
// #else
//         FilePermission FilePermissions::FromPosixMode(mode_t mode) {
//             FilePermission permissions = FilePermission::None;

//             // User permissions
//             if (mode & S_IRUSR)
//                 permissions = permissions | FilePermission::UserRead;
//             if (mode & S_IWUSR)
//                 permissions = permissions | FilePermission::UserWrite;
//             if (mode & S_IXUSR)
//                 permissions = permissions | FilePermission::UserExecute;

//             // Group permissions
//             if (mode & S_IRGRP)
//                 permissions = permissions | FilePermission::GroupRead;
//             if (mode & S_IWGRP)
//                 permissions = permissions | FilePermission::GroupWrite;
//             if (mode & S_IXGRP)
//                 permissions = permissions | FilePermission::GroupExecute;

//             // Other permissions
//             if (mode & S_IROTH)
//                 permissions = permissions | FilePermission::OtherRead;
//             if (mode & S_IWOTH)
//                 permissions = permissions | FilePermission::OtherWrite;
//             if (mode & S_IXOTH)
//                 permissions = permissions | FilePermission::OtherExecute;

//             return permissions;
//         }

//         mode_t FilePermissions::ToPosixMode(FilePermission permissions) {
//             mode_t mode = 0;

//             // User permissions
//             if ((permissions & FilePermission::UserRead) !=
//                 FilePermission::None)
//                 mode |= S_IRUSR;
//             if ((permissions & FilePermission::UserWrite) !=
//                 FilePermission::None)
//                 mode |= S_IWUSR;
//             if ((permissions & FilePermission::UserExecute) !=
//                 FilePermission::None)
//                 mode |= S_IXUSR;

//             // Group permissions
//             if ((permissions & FilePermission::GroupRead) !=
//                 FilePermission::None)
//                 mode |= S_IRGRP;
//             if ((permissions & FilePermission::GroupWrite) !=
//                 FilePermission::None)
//                 mode |= S_IWGRP;
//             if ((permissions & FilePermission::GroupExecute) !=
//                 FilePermission::None)
//                 mode |= S_IXGRP;

//             // Other permissions
//             if ((permissions & FilePermission::OtherRead) !=
//                 FilePermission::None)
//                 mode |= S_IROTH;
//             if ((permissions & FilePermission::OtherWrite) !=
//                 FilePermission::None)
//                 mode |= S_IWOTH;
//             if ((permissions & FilePermission::OtherExecute) !=
//                 FilePermission::None)
//                 mode |= S_IXOTH;

//             return mode;
//         }
// #endif

//         bool FilePermissions::CheckAccess(const std::string& path,
//                                           FilePermission permission) {
//             FilePermission currentPermissions = GetPermissions(path);
//             return (currentPermissions & permission) != FilePermission::None;
//         }

//     }  // namespace FileSystem
// }  // namespace Engine
