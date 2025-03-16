#include "FileSystem/FileSystem.h"

#include <memory>

#include "FileSystem/StandardFileSystem.h"

namespace Engine {

    std::unique_ptr<IFileSystem> FileSystem::s_instance;

    std::unique_ptr<IFileSystem> FileSystemFactory::CreateFileSystem() {
        return std::unique_ptr<IFileSystem>(
            (IFileSystem*)(new StandardFileSystem));
    }

    std::unique_ptr<IFileSystem> FileSystemFactory::CreateVirtualFileSystem() {
        // TODO: Implement virtual file system if needed
        return nullptr;
    }

    IFileSystem& FileSystem::Get() {
        if (!s_instance) {
            Initialize();
        }
        return *s_instance;
    }

    void FileSystem::Initialize(std::unique_ptr<IFileSystem> fileSystem) {
        if (!fileSystem) {
            fileSystem = FileSystemFactory::CreateFileSystem();
        }
        s_instance = std::move(fileSystem);
    }

    void FileSystem::Shutdown() { s_instance.reset(); }

}  // namespace Engine
