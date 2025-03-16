
#pragma once

#include <vulkan/vulkan.h>

#include <filesystem>
#include <mutex>
#include <set>
#include <thread>
#include <unordered_map>

#include "RHI/RHIShaderHotReload.h"
#include "VulkanRHI.h"
#include "VulkanShaderCompiler.h"

namespace Engine {
    namespace RHI {

        // Vulkan着色器变体实现
        class VulkanShaderVariant : public IRHIShaderVariant {
          public:
            VulkanShaderVariant(VkDevice device,
                                const ShaderVariantDesc& desc,
                                const std::string& sourcePath);
            virtual ~VulkanShaderVariant();

            // IRHIShaderVariant接口实现
            virtual const ShaderVariantDesc& GetDesc() const override {
                return m_Desc;
            }
            virtual bool Compile() override;
            virtual bool IsCompiled() const override { return m_IsCompiled; }

            // Vulkan特定方法
            VkShaderModule GetVkShaderModule() const { return m_ShaderModule; }
            const std::vector<uint8>& GetSPIRVCode() const {
                return m_SPIRVCode;
            }

          private:
            VkDevice m_Device;
            ShaderVariantDesc m_Desc;
            std::string m_SourcePath;
            VkShaderModule m_ShaderModule;
            std::vector<uint8> m_SPIRVCode;
            bool m_IsCompiled;
        };

        // Vulkan着色器热重载系统实现
        class VulkanShaderHotReload : public IRHIShaderHotReload {
          public:
            VulkanShaderHotReload(VkDevice device);
            virtual ~VulkanShaderHotReload();

            // IRHIShaderHotReload接口实现
            virtual void RegisterListener(
                IRHIShaderHotReloadListener* listener) override;
            virtual void UnregisterListener(
                IRHIShaderHotReloadListener* listener) override;
            virtual void StartWatching(const std::string& directory) override;
            virtual void StopWatching() override;
            virtual void TriggerRecompile(
                const std::string& shaderPath) override;

          private:
            void WatchThread();
            void HandleFileChange(const std::filesystem::path& path);
            void NotifyListeners(IRHIShader* shader);
            void NotifyError(const std::string& error);

            VkDevice m_Device;
            std::string m_WatchDirectory;
            bool m_IsWatching;
            std::thread m_WatchThread;
            std::mutex m_ListenerMutex;
            std::set<IRHIShaderHotReloadListener*> m_Listeners;
            std::unordered_map<std::string, IRHIShader*> m_ShaderMap;
            VulkanShaderCompiler m_ShaderCompiler;
        };

        // Vulkan资源虚拟化实现
        class VulkanResourceVirtualization : public IRHIResourceVirtualization {
          public:
            VulkanResourceVirtualization(
                VkDevice device,
                VkPhysicalDevice physicalDevice,
                const ResourceVirtualizationDesc& desc);
            virtual ~VulkanResourceVirtualization();

            // IRHIResourceVirtualization接口实现
            virtual const ResourceVirtualizationDesc& GetDesc() const override {
                return m_Desc;
            }
            virtual void StreamIn(uint64 offset, uint64 size) override;
            virtual void StreamOut(uint64 offset, uint64 size) override;
            virtual void MakeResident(uint64 offset, uint64 size) override;
            virtual void Evict(uint64 offset, uint64 size) override;
            virtual void Defragment() override;
            virtual float GetResidencyPercentage() const override;
            virtual uint64 GetPhysicalUsage() const override;

          private:
            struct MemoryBlock {
                VkDeviceMemory memory;
                uint64 offset;
                uint64 size;
                bool resident;
            };

            void AllocateMemory(uint64 size);
            void FreeMemory(VkDeviceMemory memory);
            void UpdateResidency();

            VkDevice m_Device;
            VkPhysicalDevice m_PhysicalDevice;
            ResourceVirtualizationDesc m_Desc;
            std::vector<MemoryBlock> m_MemoryBlocks;
            uint64 m_TotalResidentSize;
            std::mutex m_MemoryMutex;
        };

    }  // namespace RHI
}  // namespace Engine
