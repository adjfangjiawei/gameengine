#include "FileSystem/StandardFileSystem.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <regex>
#include <system_error>

#include "UUID/UUID.h"

#if PLATFORM_WINDOWS
#include <rpc.h>
#include <windows.h>
#else
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace Engine {

    // StandardFile implementation
    StandardFile::StandardFile(FILE* handle)
        : m_handle(handle), m_closed(false) {}

    StandardFile::~StandardFile() {
        if (!m_closed) {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);
            if (!m_closed) {
                fclose(m_handle);
                m_closed = true;
            }
        }
    }

    size_t StandardFile::Read(void* buffer, size_t size) {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        if (m_closed || !buffer || size == 0) return 0;
        return fread(buffer, 1, size, m_handle);
    }

    size_t StandardFile::Write(const void* buffer, size_t size) {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        if (m_closed || !buffer || size == 0) return 0;
        return fwrite(buffer, 1, size, m_handle);
    }

    void StandardFile::Flush() {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        if (!m_closed) {
            fflush(m_handle);
        }
    }

    void StandardFile::Seek(int64_t offset, int origin) const {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        if (!m_closed) {
#if PLATFORM_WINDOWS
            _fseeki64(m_handle, offset, origin);
#else
            fseeko(m_handle, offset, origin);
#endif
        }
    }

    int64_t StandardFile::Tell() const {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        if (m_closed) return -1;
#if PLATFORM_WINDOWS
        return _ftelli64(m_handle);
#else
        return ftello(m_handle);
#endif
    }

    size_t StandardFile::GetSize() const {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        if (m_closed) return 0;

        int64_t currentPos = Tell();
        Seek(0, SEEK_END);
        int64_t size = Tell();
        Seek(currentPos, SEEK_SET);
        return static_cast<size_t>(size);
    }

    bool StandardFile::IsEOF() const {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        if (m_closed) return true;
        return feof(m_handle) != 0;
    }

    void StandardFile::Close() {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        if (!m_closed) {
            fclose(m_handle);
            m_closed = true;
        }
    }

    std::vector<uint8_t> StandardFile::ReadAll() {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        if (m_closed) return std::vector<uint8_t>();

        size_t size = GetSize();
        std::vector<uint8_t> buffer(size);
        Seek(0, SEEK_SET);
        Read(buffer.data(), size);
        return buffer;
    }

    std::string StandardFile::ReadAllText() {
        auto buffer = ReadAll();
        return std::string(buffer.begin(), buffer.end());
    }

    std::vector<std::string> StandardFile::ReadAllLines() {
        std::vector<std::string> lines;
        std::string text = ReadAllText();
        std::string line;
        std::istringstream stream(text);

        while (std::getline(stream, line)) {
            lines.push_back(line);
        }

        return lines;
    }

    bool StandardFile::WriteAllBytes(const std::vector<uint8_t>& data) {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        if (m_closed) return false;

        Seek(0, SEEK_SET);
        size_t written = Write(data.data(), data.size());
        Flush();
        return written == data.size();
    }

    bool StandardFile::WriteAllText(const std::string& text) {
        return WriteAllBytes(std::vector<uint8_t>(text.begin(), text.end()));
    }

    bool StandardFile::WriteAllLines(const std::vector<std::string>& lines) {
        std::string text;
        for (const auto& line : lines) {
            text += line + "\n";
        }
        return WriteAllText(text);
    }

    // StandardFileSystem implementation
    StandardFileSystem::StandardFileSystem() {}

    StandardFileSystem::~StandardFileSystem() {}

    // Update OpenFile with proper sharing and creation handling
    std::unique_ptr<IFile> StandardFileSystem::OpenFile(const std::string& path,
                                                        FileMode mode,
                                                        FileShare share) {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);

        try {
            // Check if path is empty
            if (path.empty()) {
                return nullptr;
            }

            // For write/append modes, ensure the directory exists
            if (mode != FileMode::Read) {
                std::string dirPath = GetDirectoryName(path);
                if (!dirPath.empty() && !Exists(dirPath)) {
                    if (!CreateDirectory(dirPath)) {
                        return nullptr;
                    }
                }
            }

            // Check file existence for different modes
            bool fileExists = Exists(path);
            if (mode == FileMode::Read && !fileExists) {
                return nullptr;  // Can't read non-existent file
            }

            const char* modeStr = GetFileModeString(mode);
            FILE* handle = nullptr;

#if PLATFORM_WINDOWS
            // Convert share mode to Windows flags
            int shFlag;
            switch (share) {
                case FileShare::Read:
                    shFlag = _SH_DENYWR;
                    break;
                case FileShare::Write:
                    shFlag = _SH_DENYRD;
                    break;
                case FileShare::ReadWrite:
                    shFlag = _SH_DENYNO;
                    break;
                default:
                    shFlag = _SH_DENYRW;
                    break;
            }

            if (_fsopen_s(&handle, path.c_str(), modeStr, shFlag) != 0) {
                return nullptr;
            }
#else
            // On Linux/Unix systems
            handle = fopen(path.c_str(), modeStr);

            if (handle && share != FileShare::None) {
                // Implement file locking using flock
                int fd = fileno(handle);
                if (fd != -1) {
                    int operation = LOCK_EX;  // Exclusive lock by default
                    if (share == FileShare::Read) {
                        operation = LOCK_SH;  // Shared lock for read sharing
                    }
                    if (flock(fd, operation | LOCK_NB) == -1) {
                        fclose(handle);
                        return nullptr;
                    }
                }
            }
#endif

            if (!handle) {
                return nullptr;
            }

            return std::make_unique<StandardFile>(handle);
        } catch (const std::exception&) {
            return nullptr;
        }
    }

    bool StandardFileSystem::CreateDirectory(const std::string& path) {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        try {
            if (Exists(path)) {
                if (IsDirectory(path)) {
                    return true;  // Directory already exists
                } else {
                    return false;  // Path exists but is not a directory
                }
            }
            return std::filesystem::create_directories(
                path);  // Create all needed parent directories
        } catch (const std::filesystem::filesystem_error& e) {
            // Log error or store error message if needed
            return false;
        }
    }

    bool StandardFileSystem::DeleteDirectory(const std::string& path,
                                             bool recursive) {
        try {
            if (recursive) {
                return RecursiveDeleteDirectory(path);
            } else {
                return std::filesystem::remove(path);
            }
        } catch (const std::filesystem::filesystem_error&) {
            return false;
        }
    }

    bool StandardFileSystem::DeleteFile(const std::string& path) {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        try {
            if (!Exists(path)) {
                return false;  // File doesn't exist
            }
            if (IsDirectory(path)) {
                return false;  // Path is a directory, not a file
            }
            return std::filesystem::remove(path);
        } catch (const std::filesystem::filesystem_error& e) {
            // Could add error logging here
            return false;
        }
    }

    bool StandardFileSystem::CopyFile(const std::string& source,
                                      const std::string& destination,
                                      bool overwrite) {
        try {
            std::filesystem::copy_options options =
                std::filesystem::copy_options::none;
            if (overwrite) {
                options |= std::filesystem::copy_options::overwrite_existing;
            }
            std::filesystem::copy(source, destination, options);
            return true;
        } catch (const std::filesystem::filesystem_error&) {
            return false;
        }
    }

    bool StandardFileSystem::MoveFile(const std::string& source,
                                      const std::string& destination) {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        try {
            if (!Exists(source)) {
                return false;  // Source file doesn't exist
            }
            if (IsDirectory(source)) {
                return false;  // Source is a directory, not a file
            }

            // If destination exists, try to delete it first
            if (Exists(destination)) {
                if (IsDirectory(destination)) {
                    return false;  // Destination is a directory
                }
                if (!DeleteFile(destination)) {
                    return false;  // Failed to delete existing destination file
                }
            }

            // Create destination directory if it doesn't exist
            std::string destDir = GetDirectoryName(destination);
            if (!destDir.empty() && !Exists(destDir)) {
                if (!CreateDirectory(destDir)) {
                    return false;  // Failed to create destination directory
                }
            }

            std::filesystem::rename(source, destination);
            return true;
        } catch (const std::filesystem::filesystem_error& e) {
            // Could add error logging here
            return false;
        }
    }

    bool StandardFileSystem::Exists(const std::string& path) const {
        return std::filesystem::exists(path);
    }

    bool StandardFileSystem::IsDirectory(const std::string& path) const {
        return std::filesystem::is_directory(path);
    }

    FileAttributes StandardFileSystem::GetAttributes(
        const std::string& path) const {
        return GetFileAttributesInternal(path);
    }

    void StandardFileSystem::OpenFileAsync(const std::string& path,
                                           FileMode mode,
                                           FileSystemCallback callback,
                                           [[maybe_unused]] FileShare share) {
        auto result = OpenFile(path, mode, share);
        if (callback) {
            callback(result != nullptr, result ? "" : "Failed to open file");
        }
    }

    void StandardFileSystem::CreateDirectoryAsync(const std::string& path,
                                                  FileSystemCallback callback) {
        bool success = CreateDirectory(path);
        if (callback) {
            callback(success, success ? "" : "Failed to create directory");
        }
    }

    void StandardFileSystem::DeleteDirectoryAsync(const std::string& path,
                                                  FileSystemCallback callback,
                                                  bool recursive) {
        bool success = DeleteDirectory(path, recursive);
        if (callback) {
            callback(success, success ? "" : "Failed to delete directory");
        }
    }

    void StandardFileSystem::DeleteFileAsync(const std::string& path,
                                             FileSystemCallback callback) {
        bool success = DeleteFile(path);
        if (callback) {
            callback(success, success ? "" : "Failed to delete file");
        }
    }

    void StandardFileSystem::CopyFileAsync(const std::string& source,
                                           const std::string& destination,
                                           FileSystemCallback callback,
                                           bool overwrite) {
        bool success = CopyFile(source, destination, overwrite);
        if (callback) {
            callback(success, success ? "" : "Failed to copy file");
        }
    }

    void StandardFileSystem::MoveFileAsync(const std::string& source,
                                           const std::string& destination,
                                           FileSystemCallback callback) {
        bool success = MoveFile(source, destination);
        if (callback) {
            callback(success, success ? "" : "Failed to move file");
        }
    }

    std::string StandardFileSystem::GetCurrentDirectory() const {
        return std::filesystem::current_path().string();
    }

    bool StandardFileSystem::SetCurrentDirectory(const std::string& path) {
        try {
            std::filesystem::current_path(path);
            return true;
        } catch (const std::filesystem::filesystem_error&) {
            return false;
        }
    }

    std::string StandardFileSystem::GetAbsolutePath(
        const std::string& path) const {
        try {
            return std::filesystem::absolute(path).string();
        } catch (const std::filesystem::filesystem_error&) {
            return path;
        }
    }

    std::string StandardFileSystem::GetCanonicalPath(
        const std::string& path) const {
        try {
            return std::filesystem::canonical(path).string();
        } catch (const std::filesystem::filesystem_error&) {
            return path;
        }
    }

    size_t StandardFileSystem::GetAvailableSpace(
        const std::string& path) const {
#if PLATFORM_WINDOWS
        ULARGE_INTEGER freeBytesAvailable;
        if (GetDiskFreeSpaceExA(
                path.c_str(), &freeBytesAvailable, nullptr, nullptr)) {
            return static_cast<size_t>(freeBytesAvailable.QuadPart);
        }
#else
        struct statvfs stat;
        if (statvfs(path.c_str(), &stat) == 0) {
            return static_cast<size_t>(stat.f_bavail) *
                   static_cast<size_t>(stat.f_frsize);
        }
#endif
        return 0;
    }

    size_t StandardFileSystem::GetTotalSpace(const std::string& path) const {
#if PLATFORM_WINDOWS
        ULARGE_INTEGER totalBytes;
        if (GetDiskFreeSpaceExA(path.c_str(), nullptr, &totalBytes, nullptr)) {
            return static_cast<size_t>(totalBytes.QuadPart);
        }
#else
        struct statvfs stat;
        if (statvfs(path.c_str(), &stat) == 0) {
            return static_cast<size_t>(stat.f_blocks) *
                   static_cast<size_t>(stat.f_frsize);
        }
#endif
        return 0;
    }

    std::string StandardFileSystem::GetFileSystemType(
        const std::string&) const {
        // This is a simplified implementation
        return "StandardFS";
    }

    bool StandardFileSystem::ReadFileToString(const std::string& path,
                                              std::string& content) {
        auto file = OpenFile(path, FileMode::Read);
        if (!file) return false;

        content = static_cast<StandardFile*>(file.get())->ReadAllText();
        return true;
    }

    bool StandardFileSystem::WriteStringToFile(const std::string& path,
                                               const std::string& content) {
        auto file = OpenFile(path, FileMode::Write);
        if (!file) return false;

        return static_cast<StandardFile*>(file.get())->WriteAllText(content);
    }

    std::vector<uint8_t> StandardFileSystem::ReadAllBytes(
        const std::string& path) {
        auto file = OpenFile(path, FileMode::Read);
        if (!file) return std::vector<uint8_t>();

        return static_cast<StandardFile*>(file.get())->ReadAll();
    }

    bool StandardFileSystem::WriteAllBytes(const std::string& path,
                                           const std::vector<uint8_t>& data) {
        auto file = OpenFile(path, FileMode::Write);
        if (!file) return false;

        return static_cast<StandardFile*>(file.get())->WriteAllBytes(data);
    }

    std::vector<std::string> StandardFileSystem::ReadAllLines(
        const std::string& path) {
        auto file = OpenFile(path, FileMode::Read);
        if (!file) return std::vector<std::string>();

        return static_cast<StandardFile*>(file.get())->ReadAllLines();
    }

    bool StandardFileSystem::WriteAllLines(
        const std::string& path, const std::vector<std::string>& lines) {
        auto file = OpenFile(path, FileMode::Write);
        if (!file) return false;

        return static_cast<StandardFile*>(file.get())->WriteAllLines(lines);
    }

    // Static utility methods
    std::string StandardFileSystem::CombinePaths(
        const std::string& path1, const std::string& path2) const {
        std::filesystem::path p1(path1);
        std::filesystem::path p2(path2);
        return (p1 / p2).string();
    }

    std::string StandardFileSystem::GetDirectoryName(
        const std::string& path) const {
        return std::filesystem::path(path).parent_path().string();
    }

    std::string StandardFileSystem::GetFileName(const std::string& path) const {
        return std::filesystem::path(path).filename().string();
    }

    std::string StandardFileSystem::GetFileNameWithoutExtension(
        const std::string& path) const {
        return std::filesystem::path(path).stem().string();
    }

    std::string StandardFileSystem::GetExtension(
        const std::string& path) const {
        return std::filesystem::path(path).extension().string();
    }

    std::string StandardFileSystem::ChangeExtension(
        const std::string& path, const std::string& extension) const {
        std::filesystem::path p(path);
        p.replace_extension(extension);
        return p.string();
    }

    bool StandardFileSystem::IsPathRooted(const std::string& path) const {
        return std::filesystem::path(path).is_absolute();
    }

    std::string StandardFileSystem::GetTempPath() const {
        return std::filesystem::temp_directory_path().string();
    }

    std::string StandardFileSystem::GetTempFileName() const {
        std::string tempPath = GetTempPath();
#if PLATFORM_WINDOWS
        UUID uuid;
        UuidCreate(&uuid);
        char* str;
        UuidToStringA(&uuid, (RPC_CSTR*)&str);
        std::string fileName = std::string("tmp_") + str + ".tmp";
        RpcStringFreeA((RPC_CSTR*)&str);
#else
        // 在非Windows平台上使用时间戳和随机数生成唯一文件名
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                             now.time_since_epoch())
                             .count();
        std::string fileName = "tmp_" + std::to_string(timestamp) + "_" +
                               std::to_string(rand()) + ".tmp";
#endif
        return CombinePaths(tempPath, fileName);
    }

    // Private helper methods
    const char* StandardFileSystem::GetFileModeString(FileMode mode) {
        switch (mode) {
            case FileMode::Read:
                return "rb";
            case FileMode::Write:
                return "wb";
            case FileMode::Append:
                return "ab";
            case FileMode::ReadWrite:
                return "r+b";
            case FileMode::ReadAppend:
                return "a+b";
            default:
                return "rb";
        }
    }

    bool StandardFileSystem::RecursiveDeleteDirectory(const std::string& path) {
        try {
            std::filesystem::remove_all(path);
            return true;
        } catch (const std::filesystem::filesystem_error&) {
            return false;
        }
    }

    bool StandardFileSystem::MatchesPattern(const std::string& name,
                                            const std::string& pattern) {
        std::string regexPattern = "^";
        for (char c : pattern) {
            switch (c) {
                case '*':
                    regexPattern += ".*";
                    break;
                case '?':
                    regexPattern += ".";
                    break;
                case '.':
                    regexPattern += "\\.";
                    break;
                default:
                    regexPattern += c;
            }
        }
        regexPattern += "$";
        try {
            return std::regex_match(name, std::regex(regexPattern));
        } catch (...) {
            return false;
        }
    }

    FileAttributes StandardFileSystem::GetFileAttributesInternal(
        const std::string& path) {
        FileAttributes attrs;
        std::error_code ec;

        try {
            // Get basic file status
            std::filesystem::file_status status =
                std::filesystem::status(path, ec);
            if (ec) {
                return attrs;
            }

            // Set basic attributes
            attrs.isDirectory = std::filesystem::is_directory(status);
            attrs.isReadOnly =
                (status.permissions() & std::filesystem::perms::owner_write) ==
                std::filesystem::perms::none;

#if PLATFORM_WINDOWS
            DWORD winAttrs = GetFileAttributesA(path.c_str());
            if (winAttrs != INVALID_FILE_ATTRIBUTES) {
                attrs.isHidden = (winAttrs & FILE_ATTRIBUTE_HIDDEN) != 0;
                attrs.isSystem = (winAttrs & FILE_ATTRIBUTE_SYSTEM) != 0;
                attrs.isArchive = (winAttrs & FILE_ATTRIBUTE_ARCHIVE) != 0;
                attrs.isTemporary = (winAttrs & FILE_ATTRIBUTE_TEMPORARY) != 0;

                // Get file times using Windows API for more precise timing
                WIN32_FILE_ATTRIBUTE_DATA fileData;
                if (GetFileAttributesExA(
                        path.c_str(), GetFileExInfoStandard, &fileData)) {
                    ULARGE_INTEGER ull;

                    // Creation time
                    ull.LowPart = fileData.ftCreationTime.dwLowDateTime;
                    ull.HighPart = fileData.ftCreationTime.dwHighDateTime;
                    attrs.creationTime = std::chrono::system_clock::from_time_t(
                        (ull.QuadPart - 116444736000000000ULL) / 10000000ULL);

                    // Last access time
                    ull.LowPart = fileData.ftLastAccessTime.dwLowDateTime;
                    ull.HighPart = fileData.ftLastAccessTime.dwHighDateTime;
                    attrs.lastAccessTime =
                        std::chrono::system_clock::from_time_t(
                            (ull.QuadPart - 116444736000000000ULL) /
                            10000000ULL);

                    // Last write time
                    ull.LowPart = fileData.ftLastWriteTime.dwLowDateTime;
                    ull.HighPart = fileData.ftLastWriteTime.dwHighDateTime;
                    attrs.lastWriteTime =
                        std::chrono::system_clock::from_time_t(
                            (ull.QuadPart - 116444736000000000ULL) /
                            10000000ULL);
                }
            }
#else
            // Unix/Linux specific attributes
            attrs.isHidden = !path.empty() && path[0] == '.';

            struct stat st;
            if (stat(path.c_str(), &st) == 0) {
                // Set times
                attrs.creationTime =
                    std::chrono::system_clock::from_time_t(st.st_ctime);
                attrs.lastAccessTime =
                    std::chrono::system_clock::from_time_t(st.st_atime);
                attrs.lastWriteTime =
                    std::chrono::system_clock::from_time_t(st.st_mtime);

                // Set permissions
                attrs.isReadOnly = (st.st_mode & S_IWUSR) == 0;
                attrs.isSystem = false;     // No direct equivalent in Unix
                attrs.isTemporary = false;  // No direct equivalent in Unix
                attrs.isArchive = false;    // No direct equivalent in Unix
            }
#endif

            // Get file size
            if (!attrs.isDirectory) {
                attrs.size = std::filesystem::file_size(path, ec);
                if (ec) {
                    attrs.size = 0;
                }
            }

            return attrs;
        } catch (const std::exception&) {
            return attrs;
        }
    }

    std::vector<FileSystemEntry> StandardFileSystem::ListDirectory(
        const std::string& path, const std::string& pattern) const {
        std::vector<FileSystemEntry> entries;
        std::lock_guard<std::recursive_mutex> lock(m_mutex);

        try {
            // Check if directory exists
            if (!Exists(path) || !IsDirectory(path)) {
                return entries;
            }

            // Use directory_iterator with error code for better error handling
            std::error_code ec;
            for (const auto& entry :
                 std::filesystem::directory_iterator(path, ec)) {
                if (ec) {
                    continue;  // Skip entries that can't be accessed
                }

                std::string filename = entry.path().filename().string();
                if (StandardFileSystem::MatchesPattern(filename, pattern)) {
                    FileSystemEntry fsEntry;
                    fsEntry.name = filename;
                    fsEntry.fullPath = entry.path().string();

                    // Get detailed attributes for each entry
                    fsEntry.attributes =
                        GetFileAttributesInternal(entry.path().string());

                    // Additional error handling for specific operations
                    std::error_code sizeEc;
                    if (entry.is_regular_file(ec)) {
                        auto fileSize = entry.file_size(sizeEc);
                        if (!sizeEc) {
                            fsEntry.attributes.size = fileSize;
                        }
                    }

                    entries.push_back(std::move(fsEntry));
                }
            }

            // Sort entries (directories first, then files alphabetically)
            std::sort(
                entries.begin(),
                entries.end(),
                [](const FileSystemEntry& a, const FileSystemEntry& b) {
                    if (a.attributes.isDirectory != b.attributes.isDirectory) {
                        return a.attributes.isDirectory >
                               b.attributes.isDirectory;
                    }
                    return a.name < b.name;
                });

        } catch (const std::exception&) {
            // Return whatever entries we managed to collect
        }

        return entries;
    }

    std::unique_ptr<IFile> OpenFile(const std::string& path,
                                    FileMode mode,
                                    FileShare share) {
        static StandardFileSystem fs;
        return fs.OpenFile(path, mode, share);
    }

}  // namespace Engine
