#pragma once

#include <chrono>
#include <fstream>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "FileSystem/FilePermissions.h"
#include "CoreTypes.h"
#include "String/StringUtils.h"

namespace Engine {

    /**
     * @brief File access modes
     */
    enum class FileMode {
        Read,       // Open for reading
        Write,      // Open for writing (create or truncate)
        Append,     // Open for appending
        ReadWrite,  // Open for both reading and writing
        ReadAppend  // Open for both reading and appending
    };

    /**
     * @brief File sharing modes
     */
    enum class FileShare {
        None,      // No sharing
        Read,      // Allow others to read
        Write,     // Allow others to write
        ReadWrite  // Allow others to read and write
    };

    /**
     * @brief File creation modes
     */
    enum class FileCreation {
        New,           // Create new file, fail if exists
        CreateAlways,  // Always create new file
        OpenExisting,  // Open existing file, fail if doesn't exist
        OpenAlways,    // Open if exists, create if doesn't
        TruncateExist  // Open and truncate existing file
    };

    /**
     * @brief File attributes structure
     */
    struct FileAttributes {
        bool isDirectory = false;
        bool isReadOnly = false;
        bool isHidden = false;
        bool isSystem = false;
        bool isTemporary = false;
        bool isArchive = false;
        size_t size = 0;
        std::chrono::system_clock::time_point creationTime;
        std::chrono::system_clock::time_point lastAccessTime;
        std::chrono::system_clock::time_point lastWriteTime;
    };

    /**
     * @brief File system entry information
     */
    struct FileSystemEntry {
        std::string name;
        std::string fullPath;
        FileAttributes attributes;
    };

    /**
     * @brief Callback type for asynchronous operations
     */
    using FileSystemCallback =
        std::function<void(bool success, const std::string &error)>;

    /**
     * @brief Interface for file operations
     */
    class IFile {
      public:
        virtual ~IFile() = default;

        // Basic operations
        virtual size_t Read(void *buffer, size_t size) = 0;
        virtual size_t Write(const void *buffer, size_t size) = 0;
        virtual void Flush() = 0;
        virtual void Seek(int64_t offset, int origin) const = 0;
        virtual int64_t Tell() const = 0;
        virtual size_t GetSize() const = 0;
        virtual bool IsEOF() const = 0;
        virtual void Close() = 0;
    };

    /**
     * @brief Interface for file system operations
     */
    class IFileSystem {
      public:
        virtual ~IFileSystem() = default;

        // Synchronous operations
        virtual std::unique_ptr<IFile> OpenFile(
            const std::string &path,
            FileMode mode,
            FileShare share = FileShare::None) = 0;

        virtual bool CreateDirectory(const std::string &path) = 0;
        virtual bool DeleteDirectory(const std::string &path,
                                     bool recursive = false) = 0;
        virtual bool DeleteFile(const std::string &path) = 0;
        virtual bool CopyFile(const std::string &source,
                              const std::string &destination,
                              bool overwrite = false) = 0;
        virtual bool MoveFile(const std::string &source,
                              const std::string &destination) = 0;

        virtual bool Exists(const std::string &path) const = 0;
        virtual bool IsDirectory(const std::string &path) const = 0;
        virtual FileAttributes GetAttributes(const std::string &path) const = 0;

        virtual std::vector<FileSystemEntry> ListDirectory(
            const std::string &path,
            const std::string &pattern = "*") const = 0;

        // Asynchronous operations
        virtual void OpenFileAsync(const std::string &path,
                                   FileMode mode,
                                   FileSystemCallback callback,
                                   FileShare share = FileShare::None) = 0;

        virtual void CreateDirectoryAsync(const std::string &path,
                                          FileSystemCallback callback) = 0;

        virtual void DeleteDirectoryAsync(const std::string &path,
                                          FileSystemCallback callback,
                                          bool recursive = false) = 0;

        virtual void DeleteFileAsync(const std::string &path,
                                     FileSystemCallback callback) = 0;

        virtual void CopyFileAsync(const std::string &source,
                                   const std::string &destination,
                                   FileSystemCallback callback,
                                   bool overwrite = false) = 0;

        virtual void MoveFileAsync(const std::string &source,
                                   const std::string &destination,
                                   FileSystemCallback callback) = 0;

        // Path operations
        virtual std::string GetCurrentDirectory() const = 0;
        virtual bool SetCurrentDirectory(const std::string &path) = 0;
        virtual std::string GetAbsolutePath(const std::string &path) const = 0;
        virtual std::string GetCanonicalPath(const std::string &path) const = 0;

        // File system information
        virtual size_t GetAvailableSpace(const std::string &path) const = 0;
        virtual size_t GetTotalSpace(const std::string &path) const = 0;
        virtual std::string GetFileSystemType(
            const std::string &path) const = 0;

        // Path utility methods
        virtual std::string CombinePaths(const std::string &path1,
                                         const std::string &path2) const = 0;
        virtual std::string GetDirectoryName(const std::string &path) const = 0;
        virtual std::string GetFileName(const std::string &path) const = 0;
        virtual std::string GetFileNameWithoutExtension(
            const std::string &path) const = 0;
        virtual std::string GetExtension(const std::string &path) const = 0;
        virtual std::string ChangeExtension(
            const std::string &path, const std::string &extension) const = 0;
        virtual bool IsPathRooted(const std::string &path) const = 0;
        virtual std::string GetTempPath() const = 0;
        virtual std::string GetTempFileName() const = 0;
    };

    /**
     * @brief Factory for creating file system instances
     */
    class FileSystemFactory {
      public:
        static std::unique_ptr<IFileSystem> CreateFileSystem();
        static std::unique_ptr<IFileSystem> CreateVirtualFileSystem();
    };

    /**
     * @brief Global file system access
     */
    class FileSystem {
      public:
        static bool ReadFileToString(const std::string &path,
                                     std::string &content) {
            std::ifstream file(path);
            if (!file.is_open()) {
                return false;
            }
            content.assign(std::istreambuf_iterator<char>(file),
                           std::istreambuf_iterator<char>());
            return true;
        }

        static bool WriteStringToFile(const std::string &path,
                                      const std::string &content) {
            std::ofstream file(path);
            if (!file.is_open()) {
                return false;
            }
            file << content;
            return file.good();
        }

        static IFileSystem &Get();
        static void Initialize(
            std::unique_ptr<IFileSystem> fileSystem = nullptr);
        static void Shutdown();

        // File Permissions Management
        static bool SetPermissions(const std::string &path,
                                   FilePermission permissions);
        static FilePermission GetPermissions(const std::string &path);

        // File Ownership Management
        static bool SetOwner(const std::string &path,
                             uint32 userId,
                             uint32 groupId);
        static bool SetOwner(const std::string &path,
                             const std::string &userName,
                             const std::string &groupName);
        static FileOwner GetOwner(const std::string &path);

        // Permission Check Methods
        static bool CanRead(const std::string &path);
        static bool CanWrite(const std::string &path);
        static bool CanExecute(const std::string &path);

        // Platform-specific Permission Conversions
#if PLATFORM_WINDOWS
        static FilePermission FromWindowsAttributes(uint32 attributes);
        static uint32 ToWindowsAttributes(FilePermission permissions);
#else
        static FilePermission FromPosixMode(mode_t mode);
        static mode_t ToPosixMode(FilePermission permissions);
#endif

      private:
        static std::unique_ptr<IFileSystem> s_instance;
        static bool CheckAccess(const std::string &path,
                                FilePermission permission);
    };

}  // namespace Engine
