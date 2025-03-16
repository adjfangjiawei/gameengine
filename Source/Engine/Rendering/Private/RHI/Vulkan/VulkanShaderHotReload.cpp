
#include "VulkanShaderHotReload.h"

#include <cassert>
#include <chrono>
#include <fstream>

namespace Engine {
    namespace RHI {

        // VulkanShaderVariant实现
        VulkanShaderVariant::VulkanShaderVariant(VkDevice device,
                                                 const ShaderVariantDesc& desc,
                                                 const std::string& sourcePath)
            : m_Device(device),
              m_Desc(desc),
              m_SourcePath(sourcePath),
              m_ShaderModule(VK_NULL_HANDLE),
              m_IsCompiled(false) {}

        VulkanShaderVariant::~VulkanShaderVariant() {
            if (m_ShaderModule != VK_NULL_HANDLE) {
                vkDestroyShaderModule(m_Device, m_ShaderModule, nullptr);
            }
        }

        bool VulkanShaderVariant::Compile() {
            // 设置编译选项
            ShaderCompileOptions options;
            options.Target = EShaderTarget::SPIRV;
            options.EntryPoint = m_Desc.EntryPoint;
            options.Defines = m_Desc.Defines;
            options.SourceFile = m_SourcePath;
            options.Debug =
                (static_cast<uint32>(m_Desc.Flags) &
                 static_cast<uint32>(EShaderVariantFlags::Debug)) != 0;

            // 编译着色器
            VulkanShaderCompiler compiler;
            ShaderCompileResult result = compiler.Compile(options);

            if (!result.Success) {
                return false;
            }

            // 存储SPIR-V代码
            m_SPIRVCode = std::move(result.ByteCode);

            // 创建着色器模块
            VkShaderModuleCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createInfo.codeSize = m_SPIRVCode.size();
            createInfo.pCode =
                reinterpret_cast<const uint32_t*>(m_SPIRVCode.data());

            if (vkCreateShaderModule(
                    m_Device, &createInfo, nullptr, &m_ShaderModule) !=
                VK_SUCCESS) {
                return false;
            }

            m_IsCompiled = true;
            return true;
        }

        // VulkanShaderHotReload实现
        VulkanShaderHotReload::VulkanShaderHotReload(VkDevice device)
            : m_Device(device), m_IsWatching(false) {}

        VulkanShaderHotReload::~VulkanShaderHotReload() { StopWatching(); }

        void VulkanShaderHotReload::RegisterListener(
            IRHIShaderHotReloadListener* listener) {
            std::lock_guard<std::mutex> lock(m_ListenerMutex);
            m_Listeners.insert(listener);
        }

        void VulkanShaderHotReload::UnregisterListener(
            IRHIShaderHotReloadListener* listener) {
            std::lock_guard<std::mutex> lock(m_ListenerMutex);
            m_Listeners.erase(listener);
        }

        void VulkanShaderHotReload::StartWatching(
            const std::string& directory) {
            if (m_IsWatching) {
                return;
            }

            m_WatchDirectory = directory;
            m_IsWatching = true;
            m_WatchThread =
                std::thread(&VulkanShaderHotReload::WatchThread, this);
        }

        void VulkanShaderHotReload::StopWatching() {
            if (!m_IsWatching) {
                return;
            }

            m_IsWatching = false;
            if (m_WatchThread.joinable()) {
                m_WatchThread.join();
            }
        }

        void VulkanShaderHotReload::TriggerRecompile(
            const std::string& shaderPath) {
            HandleFileChange(std::filesystem::path(shaderPath));
        }

        void VulkanShaderHotReload::WatchThread() {
            std::filesystem::path watchPath(m_WatchDirectory);

            while (m_IsWatching) {
                try {
                    for (const auto& entry :
                         std::filesystem::directory_iterator(watchPath)) {
                        if (entry.is_regular_file()) {
                            auto extension = entry.path().extension();
                            if (extension == ".glsl" || extension == ".vert" ||
                                extension == ".frag" || extension == ".comp") {
                                auto lastWriteTime =
                                    std::filesystem::last_write_time(
                                        entry.path());
                                static std::unordered_map<
                                    std::string,
                                    std::filesystem::file_time_type>
                                    lastModifiedTimes;

                                auto it = lastModifiedTimes.find(
                                    entry.path().string());
                                if (it == lastModifiedTimes.end() ||
                                    it->second != lastWriteTime) {
                                    HandleFileChange(entry.path());
                                    lastModifiedTimes[entry.path().string()] =
                                        lastWriteTime;
                                }
                            }
                        }
                    }
                } catch (const std::exception& e) {
                    NotifyError("File watching error: " +
                                std::string(e.what()));
                }

                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }

        void VulkanShaderHotReload::HandleFileChange(
            const std::filesystem::path& path) {
            auto it = m_ShaderMap.find(path.string());
            if (it == m_ShaderMap.end()) {
                return;
            }

            IRHIShader* shader = it->second;
            if (!shader) {
                return;
            }

            // 重新编译着色器
            try {
                VulkanShaderVariant* variant =
                    dynamic_cast<VulkanShaderVariant*>(shader);
                if (variant && variant->Compile()) {
                    NotifyListeners(shader);
                }
            } catch (const std::exception& e) {
                NotifyError("Shader recompilation error: " +
                            std::string(e.what()));
            }
        }

        void VulkanShaderHotReload::NotifyListeners(IRHIShader* shader) {
            std::lock_guard<std::mutex> lock(m_ListenerMutex);
            for (auto listener : m_Listeners) {
                listener->OnShaderRecompiled(shader);
            }
        }

        void VulkanShaderHotReload::NotifyError(const std::string& error) {
            std::lock_guard<std::mutex> lock(m_ListenerMutex);
            for (auto listener : m_Listeners) {
                listener->OnShaderCompileError(error);
            }
        }

        // VulkanResourceVirtualization实现
        VulkanResourceVirtualization::VulkanResourceVirtualization(
            VkDevice device,
            VkPhysicalDevice physicalDevice,
            const ResourceVirtualizationDesc& desc)
            : m_Device(device),
              m_PhysicalDevice(physicalDevice),
              m_Desc(desc),
              m_TotalResidentSize(0) {}

        VulkanResourceVirtualization::~VulkanResourceVirtualization() {
            std::lock_guard<std::mutex> lock(m_MemoryMutex);
            for (const auto& block : m_MemoryBlocks) {
                if (block.memory != VK_NULL_HANDLE) {
                    vkFreeMemory(m_Device, block.memory, nullptr);
                }
            }
        }

        void VulkanResourceVirtualization::StreamIn(
            [[maybe_unused]] uint64 offset, uint64 size) {
            std::lock_guard<std::mutex> lock(m_MemoryMutex);

            // 分配新的内存块
            AllocateMemory(size);

            // 更新驻留状态
            UpdateResidency();
        }

        void VulkanResourceVirtualization::StreamOut(uint64 offset,
                                                     uint64 size) {
            std::lock_guard<std::mutex> lock(m_MemoryMutex);

            // 查找并释放对应的内存块
            for (auto it = m_MemoryBlocks.begin();
                 it != m_MemoryBlocks.end();) {
                if (it->offset >= offset &&
                    it->offset + it->size <= offset + size) {
                    FreeMemory(it->memory);
                    it = m_MemoryBlocks.erase(it);
                } else {
                    ++it;
                }
            }

            UpdateResidency();
        }

        void VulkanResourceVirtualization::MakeResident(uint64 offset,
                                                        uint64 size) {
            std::lock_guard<std::mutex> lock(m_MemoryMutex);

            for (auto& block : m_MemoryBlocks) {
                if (block.offset >= offset &&
                    block.offset + block.size <= offset + size) {
                    block.resident = true;
                }
            }

            UpdateResidency();
        }

        void VulkanResourceVirtualization::Evict(uint64 offset, uint64 size) {
            std::lock_guard<std::mutex> lock(m_MemoryMutex);

            for (auto& block : m_MemoryBlocks) {
                if (block.offset >= offset &&
                    block.offset + block.size <= offset + size) {
                    block.resident = false;
                }
            }

            UpdateResidency();
        }

        void VulkanResourceVirtualization::Defragment() {
            std::lock_guard<std::mutex> lock(m_MemoryMutex);

            // 对内存块进行排序
            std::sort(m_MemoryBlocks.begin(),
                      m_MemoryBlocks.end(),
                      [](const MemoryBlock& a, const MemoryBlock& b) {
                          return a.offset < b.offset;
                      });

            // 合并相邻的内存块
            for (size_t i = 0; i < m_MemoryBlocks.size() - 1;) {
                auto& current = m_MemoryBlocks[i];
                auto& next = m_MemoryBlocks[i + 1];

                if (current.offset + current.size == next.offset &&
                    current.resident == next.resident) {
                    // 合并内存块
                    current.size += next.size;
                    FreeMemory(next.memory);
                    m_MemoryBlocks.erase(m_MemoryBlocks.begin() + i + 1);
                } else {
                    ++i;
                }
            }
        }

        float VulkanResourceVirtualization::GetResidencyPercentage() const {
            if (m_Desc.VirtualSize == 0) return 0.0f;
            return static_cast<float>(m_TotalResidentSize) /
                   static_cast<float>(m_Desc.VirtualSize) * 100.0f;
        }

        uint64 VulkanResourceVirtualization::GetPhysicalUsage() const {
            return m_TotalResidentSize;
        }

        void VulkanResourceVirtualization::AllocateMemory(uint64 size) {
            VkMemoryAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = size;
            allocInfo.memoryTypeIndex = 0;  // TODO: 选择合适的内存类型

            VkDeviceMemory memory;
            VkResult result =
                vkAllocateMemory(m_Device, &allocInfo, nullptr, &memory);
            if (result == VK_SUCCESS) {
                MemoryBlock block = {};
                block.memory = memory;
                block.size = size;
                block.resident = true;
                m_MemoryBlocks.push_back(block);
            }
        }

        void VulkanResourceVirtualization::FreeMemory(VkDeviceMemory memory) {
            if (memory != VK_NULL_HANDLE) {
                vkFreeMemory(m_Device, memory, nullptr);
            }
        }

        void VulkanResourceVirtualization::UpdateResidency() {
            m_TotalResidentSize = 0;
            for (const auto& block : m_MemoryBlocks) {
                if (block.resident) {
                    m_TotalResidentSize += block.size;
                }
            }
        }

    }  // namespace RHI
}  // namespace Engine
