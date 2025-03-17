
#pragma once

#include "RHI/RHIModule.h"
#include "VulkanCommandPoolManager.h"
#include "VulkanDevice.h"
#include "VulkanInstance.h"
#include "VulkanMemory.h"
#include "VulkanPhysicalDevice.h"

namespace Engine {
    namespace RHI {

        class VulkanRHI {
          public:
            static VulkanRHI& Get();

            bool Initialize(const RHIModuleInitParams& params);
            void Shutdown();

            // 获取各个组件
            VulkanInstance& GetInstance() { return Instance; }
            VulkanPhysicalDevice& GetPhysicalDevice() { return PhysicalDevice; }
            VulkanDevice* GetDevice() { return &Device; }
            VulkanCommandPoolManager& GetCommandPoolManager() {
                return CommandPoolManager;
            }
            VulkanMemoryAllocator* GetMemoryAllocator() const {
                return PhysicalDevice.GetMemoryAllocator();
            }

            // 获取Vulkan句柄
            VkInstance GetVkInstance() const { return Instance.GetHandle(); }
            VkPhysicalDevice GetVkPhysicalDevice() const {
                return PhysicalDevice.GetHandle();
            }
            const VkDevice* GetVkDevice() const { return Device.GetHandle(); }

          private:
            VulkanRHI() = default;
            ~VulkanRHI() = default;
            VulkanRHI(const VulkanRHI&) = delete;
            VulkanRHI& operator=(const VulkanRHI&) = delete;

          private:
            VulkanInstance Instance;
            VulkanPhysicalDevice PhysicalDevice;
            VulkanDevice Device;
            VulkanCommandPoolManager CommandPoolManager;
        };

    }  // namespace RHI
}  // namespace Engine
