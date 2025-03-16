#pragma once

#include <mutex>

#include "FileSystem/FilePermissions.h"
#include "FileSystem/FileSystem.h"
// #include "Threading/ThreadPool.h"

namespace Engine {

    /**
     * @brief Standard file implementation
     */
    class StandardFile : public IFile {
      public:
        StandardFile(FILE *handle);
        ~StandardFile() override;

        // IFile interface implementation
        size_t Read(void *buffer, size_t size) override;
        size_t Write(const void *buffer, size_t size) override;
        void Flush() override;
        void Seek(int64_t offset, int origin) const override;
        int64_t Tell() const override;
        size_t GetSize() const override;
        bool IsEOF() const override;
        void Close() override;

        // Additional utility methods
        std::vector<uint8_t> ReadAll();
        std::string ReadAllText();
        std::vector<std::string> ReadAllLines();
        bool WriteAllBytes(const std::vector<uint8_t> &data);
        bool WriteAllText(const std::string &text);
        bool WriteAllLines(const std::vector<std::string> &lines);

      private:
        FILE *m_handle;
        mutable std::recursive_mutex m_mutex;
        bool m_closed;
    };

    /**
     * @brief Standard file system implementation
     */
    class StandardFileSystem : public IFileSystem {
      public:
        StandardFileSystem();
        ~StandardFileSystem() override;

        // Synchronous operations
        std::unique_ptr<IFile> OpenFile(
            const std::string &path,
            FileMode mode,
            FileShare share = FileShare::None) override;

        bool CreateDirectory(const std::string &path) override;
        bool DeleteDirectory(const std::string &path,
                             bool recursive = false) override;
        bool DeleteFile(const std::string &path) override;
        bool CopyFile(const std::string &source,
                      const std::string &destination,
                      bool overwrite = false) override;
        bool MoveFile(const std::string &source,
                      const std::string &destination) override;

        bool Exists(const std::string &path) const override;
        bool IsDirectory(const std::string &path) const override;
        FileAttributes GetAttributes(const std::string &path) const override;

        std::vector<FileSystemEntry> ListDirectory(
            const std::string &path,
            const std::string &pattern = "*") const override;

        // Asynchronous operations
        void OpenFileAsync(const std::string &path,
                           FileMode mode,
                           FileSystemCallback callback,
                           FileShare share = FileShare::None) override;

        void CreateDirectoryAsync(const std::string &path,
                                  FileSystemCallback callback) override;

        void DeleteDirectoryAsync(const std::string &path,
                                  FileSystemCallback callback,
                                  bool recursive = false) override;

        void DeleteFileAsync(const std::string &path,
                             FileSystemCallback callback) override;

        void CopyFileAsync(const std::string &source,
                           const std::string &destination,
                           FileSystemCallback callback,
                           bool overwrite = false) override;

        void MoveFileAsync(const std::string &source,
                           const std::string &destination,
                           FileSystemCallback callback) override;

        // Path operations
        std::string GetCurrentDirectory() const override;
        bool SetCurrentDirectory(const std::string &path) override;
        std::string GetAbsolutePath(const std::string &path) const override;
        std::string GetCanonicalPath(const std::string &path) const override;

        // File system information
        size_t GetAvailableSpace(const std::string &path) const override;
        size_t GetTotalSpace(const std::string &path) const override;
        std::string GetFileSystemType(const std::string &path) const override;

        // Additional utility methods
        bool ReadFileToString(const std::string &path, std::string &content);
        bool WriteStringToFile(const std::string &path,
                               const std::string &content);
        std::vector<uint8_t> ReadAllBytes(const std::string &path);
        bool WriteAllBytes(const std::string &path,
                           const std::vector<uint8_t> &data);
        std::vector<std::string> ReadAllLines(const std::string &path);
        bool WriteAllLines(const std::string &path,
                           const std::vector<std::string> &lines);

        // IFileSystem path utility methods implementation
        std::string CombinePaths(const std::string &path1,
                                 const std::string &path2) const override;
        std::string GetDirectoryName(const std::string &path) const override;
        std::string GetFileName(const std::string &path) const override;
        std::string GetFileNameWithoutExtension(
            const std::string &path) const override;
        std::string GetExtension(const std::string &path) const override;
        std::string ChangeExtension(
            const std::string &path,
            const std::string &extension) const override;
        bool IsPathRooted(const std::string &path) const override;
        std::string GetTempPath() const override;
        std::string GetTempFileName() const override;

      public:
        // Helper functions
        static const char *GetFileModeString(FileMode mode);
        static bool RecursiveDeleteDirectory(const std::string &path);
        static FileAttributes GetFileAttributesInternal(
            const std::string &path);
        static std::chrono::system_clock::time_point FileTimeToSystemTime(
            time_t time);

        // Path manipulation helpers
        static std::string NormalizePath(const std::string &path);
        static bool MatchesPattern(const std::string &name,
                                   const std::string &pattern);

      private:
        mutable std::recursive_mutex m_mutex;
    };

}  // namespace Engine
