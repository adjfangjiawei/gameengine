#include <future>
#include <iostream>
#include <thread>

#include "Assert.h"
#include "FileSystem/FileSystem.h"
#include "FileSystem/StandardFileSystem.h"

namespace Engine {
    namespace Test {

        void RunFileSystemTests() {
            std::cout << "\nRunning File System Tests:" << std::endl;

            // Initialize file system
            FileSystem::Initialize(std::make_unique<StandardFileSystem>());
            auto &fs = FileSystem::Get();

            // Create test directory structure
            const std::string testDir = "FileSystemTest";
            const std::string subDir = testDir + "/SubDir";
            const std::string testFile = testDir + "/test.txt";
            const std::string testFile2 = testDir + "/test2.txt";
            const std::string testFileInSubDir = subDir + "/test.txt";

            // Test 1: Directory Operations
            {
                std::cout << "\nTesting Directory Operations:" << std::endl;

                // Clean up any existing test directory
                if (fs.Exists(testDir)) {
                    fs.DeleteDirectory(testDir, true);
                }

                // Create directories
                bool created = fs.CreateDirectory(testDir);
                ASSERT(created, "Failed to create test directory");
                ASSERT(fs.Exists(testDir),
                       "Test directory doesn't exist after creation");
                ASSERT(fs.IsDirectory(testDir),
                       "Created path is not a directory");

                created = fs.CreateDirectory(subDir);
                ASSERT(created, "Failed to create subdirectory");
                ASSERT(fs.Exists(subDir),
                       "Subdirectory doesn't exist after creation");

                std::cout << "Directory operations tests passed!" << std::endl;
            }

            // Test 2: File Operations
            {
                std::cout << "\nTesting File Operations:" << std::endl;

                // Create and write to file
                {
                    auto file = fs.OpenFile(testFile, FileMode::Write);
                    ASSERT(file != nullptr, "Failed to create test file");

                    const std::string content = "Hello, File System!";
                    size_t written =
                        file->Write(content.c_str(), content.length());
                    ASSERT(written == content.length(),
                           "Failed to write to file");

                    file->Flush();
                    file->Close();
                }

                // Read from file
                {
                    auto file = fs.OpenFile(testFile, FileMode::Read);
                    ASSERT(file != nullptr,
                           "Failed to open test file for reading");

                    char buffer[100] = {0};
                    size_t read = file->Read(buffer, 99);
                    ASSERT(read > 0, "Failed to read from file");
                    ASSERT(std::string(buffer) == "Hello, File System!",
                           "File content mismatch");

                    file->Close();
                }

                std::cout << "File operations tests passed!" << std::endl;
            }

            // Test 3: File Attributes
            {
                std::cout << "\nTesting File Attributes:" << std::endl;

                FileAttributes attrs = fs.GetAttributes(testFile);
                ASSERT(!attrs.isDirectory,
                       "File incorrectly marked as directory");
                ASSERT(attrs.size > 0, "File size is incorrect");

                attrs = fs.GetAttributes(testDir);
                ASSERT(attrs.isDirectory, "Directory not marked as directory");

                std::cout << "File attributes tests passed!" << std::endl;
            }

            // Test 4: File Copy and Move Operations
            {
                std::cout << "\nTesting File Copy and Move Operations:"
                          << std::endl;

                // Copy file
                bool copied = fs.CopyFile(testFile, testFile2, true);
                ASSERT(copied, "Failed to copy file");
                ASSERT(fs.Exists(testFile2), "Copied file doesn't exist");

                // Move file
                bool moved = fs.MoveFile(testFile2, testFileInSubDir);
                ASSERT(moved, "Failed to move file");
                ASSERT(!fs.Exists(testFile2),
                       "Source file still exists after move");
                ASSERT(fs.Exists(testFileInSubDir),
                       "Destination file doesn't exist after move");

                std::cout << "File copy and move tests passed!" << std::endl;
            }

            // Test 5: Directory Listing
            {
                std::cout << "\nTesting Directory Listing:" << std::endl;

                auto entries = fs.ListDirectory(testDir);
                ASSERT(!entries.empty(), "Directory listing is empty");

                bool foundFile = false;
                bool foundDir = false;
                for (const auto &entry : entries) {
                    if (entry.name == "test.txt") foundFile = true;
                    if (entry.name == "SubDir") foundDir = true;
                }

                ASSERT(foundFile, "File not found in directory listing");
                ASSERT(foundDir, "Subdirectory not found in directory listing");

                std::cout << "Directory listing tests passed!" << std::endl;
            }

            // Test 6: Asynchronous Operations
            {
                std::cout << "\nTesting Asynchronous Operations:" << std::endl;

                std::promise<bool> copyPromise;
                auto copyFuture = copyPromise.get_future();

                fs.CopyFileAsync(
                    testFile,
                    testFile2,
                    [&copyPromise](bool success,
                                   [[maybe_unused]] const std::string &error) {
                        copyPromise.set_value(success);
                    },
                    true);

                bool copyResult = copyFuture.get();
                ASSERT(copyResult, "Async file copy failed");
                ASSERT(fs.Exists(testFile2),
                       "File doesn't exist after async copy");

                std::cout << "Asynchronous operations tests passed!"
                          << std::endl;
            }

            // Test 7: Path Operations
            {
                std::cout << "\nTesting Path Operations:" << std::endl;

                std::string currentDir = fs.GetCurrentDirectory();
                ASSERT(!currentDir.empty(), "Failed to get current directory");

                std::string absolutePath = fs.GetAbsolutePath(testFile);
                ASSERT(!absolutePath.empty(), "Failed to get absolute path");
                ASSERT(fs.Exists(absolutePath), "Absolute path is invalid");

                std::string canonicalPath = fs.GetCanonicalPath(testFile);
                ASSERT(!canonicalPath.empty(), "Failed to get canonical path");
                ASSERT(fs.Exists(canonicalPath), "Canonical path is invalid");

                std::cout << "Path operations tests passed!" << std::endl;
            }

            // Test 8: File System Information
            {
                std::cout << "\nTesting File System Information:" << std::endl;

                size_t available = fs.GetAvailableSpace(".");
                ASSERT(available > 0, "Failed to get available space");

                size_t total = fs.GetTotalSpace(".");
                ASSERT(total > 0, "Failed to get total space");
                ASSERT(total >= available,
                       "Total space less than available space");

                std::string fsType = fs.GetFileSystemType(".");
                ASSERT(!fsType.empty(), "Failed to get file system type");

                std::cout << "File system information tests passed!"
                          << std::endl;
            }

            // Clean up
            {
                std::cout << "\nCleaning up test files and directories..."
                          << std::endl;

                fs.DeleteDirectory(testDir, true);
                ASSERT(!fs.Exists(testDir),
                       "Failed to clean up test directory");
            }

            FileSystem::Shutdown();
            std::cout << "\nAll file system tests completed successfully!"
                      << std::endl;
        }

    }  // namespace Test
}  // namespace Engine

// Add main function
int main() {
    try {
        Engine::Test::RunFileSystemTests();
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
