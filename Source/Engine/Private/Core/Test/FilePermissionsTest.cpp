
#include <string>

#include "Core/FileSystem/FilePermissions.h"
#include "Core/FileSystem/FileSystem.h"
#include "Core/Log/LogSystem.h"

using namespace Engine;

namespace Engine {
    namespace Test {
// Test helper macros
#define TEST_CASE(name) LOG_INFO("Starting test case: {}", name)

#define TEST_ASSERT(condition, message)            \
    do {                                           \
        if (!(condition)) {                        \
            LOG_ERROR("Test failed: {}", message); \
            return 1;                              \
        }                                          \
    } while (0)

        // Test basic file permissions
        int TestBasicPermissions() {
            TEST_CASE("Basic Permissions");

            // Create a test file
            const std::string testFile = "test_permissions.txt";
            {
                auto file = FileSystem::Get().OpenFile(
                    testFile, FileMode::Write, FileShare::None);
                TEST_ASSERT(file != nullptr, "Failed to create test file");
                file->Write("Test content", 12);
                file->Close();
            }

            // Test default permissions
            FilePermission perms = FileSystem::GetPermissions(testFile);
            TEST_ASSERT(
                (perms & FilePermission::UserRead) == FilePermission::UserRead,
                "File should be readable by user");
            TEST_ASSERT((perms & FilePermission::UserWrite) ==
                            FilePermission::UserWrite,
                        "File should be writable by user");

            // Test permission modification
            TEST_ASSERT(
                FileSystem::SetPermissions(testFile, FilePermission::UserRead),
                "Failed to set read-only permission");

            perms = FileSystem::GetPermissions(testFile);
            TEST_ASSERT((perms & FilePermission::UserWrite) !=
                            FilePermission::UserWrite,
                        "File should not be writable after permission change");

            // Test permission checks
            TEST_ASSERT(FileSystem::CanRead(testFile),
                        "Should be able to read the file");
            TEST_ASSERT(!FileSystem::CanWrite(testFile),
                        "Should not be able to write to read-only file");

            // Cleanup
            FileSystem::Get().DeleteFile(testFile);

            return 0;
        }

        // Test directory permissions
        int TestDirectoryPermissions() {
            TEST_CASE("Directory Permissions");

            const std::string testDir = "test_permissions_dir";

            // Create test directory
            TEST_ASSERT(FileSystem::Get().CreateDirectory(testDir),
                        "Failed to create test directory");

            // Test default directory permissions
            FilePermission perms = FileSystem::GetPermissions(testDir);
            TEST_ASSERT((perms & FilePermission::DefaultDir) ==
                            FilePermission::DefaultDir,
                        "Directory should have default permissions");

            // Modify directory permissions
            FilePermission newPerms =
                FilePermission::UserRead | FilePermission::UserExecute;
            TEST_ASSERT(FileSystem::SetPermissions(testDir, newPerms),
                        "Failed to set directory permissions");

            perms = FileSystem::GetPermissions(testDir);
            TEST_ASSERT(
                (perms & FilePermission::UserWrite) !=
                    FilePermission::UserWrite,
                "Directory should not be writable after permission change");

            // Cleanup
            FileSystem::Get().DeleteDirectory(testDir);

            return 0;
        }

        // Test ownership
        int TestOwnership() {
            TEST_CASE("File Ownership");

            const std::string testFile = "test_ownership.txt";

            // Create test file
            {
                auto file = FileSystem::Get().OpenFile(
                    testFile, FileMode::Write, FileShare::None);
                TEST_ASSERT(file != nullptr, "Failed to create test file");
                file->Write("Test content", 12);
                file->Close();
            }

            // Get current owner
            FileOwner owner = FileSystem::GetOwner(testFile);
            TEST_ASSERT(!owner.userName.empty(),
                        "Should be able to get owner name");
            TEST_ASSERT(!owner.groupName.empty(),
                        "Should be able to get group name");

            // Note: Changing ownership usually requires root privileges
            // We'll just verify that the API calls work without checking the
            // results
            FileSystem::SetOwner(testFile, owner.userId, owner.groupId);
            FileSystem::SetOwner(testFile, owner.userName, owner.groupName);

            // Cleanup
            FileSystem::Get().DeleteFile(testFile);

            return 0;
        }

        // Test permission inheritance
        int TestPermissionInheritance() {
            TEST_CASE("Permission Inheritance");

            const std::string testDir = "test_inherit_dir";
            const std::string testFile = "test_inherit_dir/test.txt";

            // Create directory with specific permissions
            TEST_ASSERT(FileSystem::Get().CreateDirectory(testDir),
                        "Failed to create test directory");
            TEST_ASSERT(
                FileSystem::SetPermissions(testDir, FilePermission::DefaultDir),
                "Failed to set directory permissions");

            // Create file in directory
            {
                auto file = FileSystem::Get().OpenFile(
                    testFile, FileMode::Write, FileShare::None);
                TEST_ASSERT(file != nullptr, "Failed to create test file");
                file->Write("Test content", 12);
                file->Close();
            }

            // Check file permissions
            FilePermission filePerms = FileSystem::GetPermissions(testFile);
            TEST_ASSERT((filePerms & FilePermission::DefaultFile) ==
                            FilePermission::DefaultFile,
                        "File should inherit default permissions");

            // Cleanup
            FileSystem::Get().DeleteFile(testFile);
            FileSystem::Get().DeleteDirectory(testDir);

            return 0;
        }

        int main() {
            LOG_INFO("Starting file permissions tests...");

            int result = 0;

            // Run all tests
            result |= TestBasicPermissions();
            result |= TestDirectoryPermissions();
            result |= TestOwnership();
            result |= TestPermissionInheritance();

            if (result == 0) {
                LOG_INFO("All file permissions tests passed!");
            } else {
                LOG_ERROR("Some file permissions tests failed!");
            }

            return result;
        }

    }  // namespace Test
}  // namespace Engine

// Global main function that calls into the test namespace
int main() { return Engine::Test::main(); }
