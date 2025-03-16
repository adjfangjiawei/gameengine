
#pragma once

#include <vulkan/vulkan.h>

#include <memory>
#include <unordered_map>
#include <vector>

#include "Rendering/RHI/RHI.h"

namespace Engine {
    namespace RHI {

        // Vulkan实例包装器
        class VulkanInstance {
          public:
            VulkanInstance() = default;
            ~VulkanInstance();

            bool Initialize(const RHIModuleInitParams& params);
            void Shutdown();

            VkInstance GetHandle() const { return Instance; }
            bool SupportsValidationLayers() const {
                return ValidationLayersEnabled;
            }

          private:
            bool CreateInstance(const RHIModuleInitParams& params);
            bool SetupDebugMessenger();
            std::vector<const char*> GetRequiredExtensions() const;
            bool CheckValidationLayerSupport() const;

            VkInstance Instance = VK_NULL_HANDLE;
            VkDebugUtilsMessengerEXT DebugMessenger = VK_NULL_HANDLE;
            bool ValidationLayersEnabled = false;

            static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
                VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                void* pUserData);
        };

        // Vulkan物理设备包装器
        class VulkanPhysicalDevice {
          public:
            struct QueueFamilyIndices {
                uint32_t GraphicsFamily = UINT32_MAX;
                uint32_t ComputeFamily = UINT32_MAX;
                uint32_t TransferFamily = UINT32_MAX;
                uint32_t PresentFamily = UINT32_MAX;

                bool IsComplete() const {
                    return GraphicsFamily != UINT32_MAX &&
                           ComputeFamily != UINT32_MAX &&
                           TransferFamily != UINT32_MAX;
                }
            };

            VulkanPhysicalDevice() = default;
            ~VulkanPhysicalDevice() = default;

            bool Initialize(VkInstance instance);

            VkPhysicalDevice GetHandle() const { return PhysicalDevice; }
            const VkPhysicalDeviceProperties& GetProperties() const {
                return Properties;
            }
            const VkPhysicalDeviceFeatures& GetFeatures() const {
                return Features;
            }
            const QueueFamilyIndices& GetQueueFamilyIndices() const {
                return QueueFamilies;
            }
            ERHIFeatureLevel GetFeatureLevel() const { return FeatureLevel; }

          private:
            bool SelectPhysicalDevice(VkInstance instance);
            int RateDeviceSuitability(VkPhysicalDevice device) const;
            QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;
            ERHIFeatureLevel DetermineFeatureLevel(
                const VkPhysicalDeviceProperties& props) const;

            VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
            VkPhysicalDeviceProperties Properties = {};
            VkPhysicalDeviceFeatures Features = {};
            QueueFamilyIndices QueueFamilies;
            ERHIFeatureLevel FeatureLevel = ERHIFeatureLevel::ES2;
        };

        // Vulkan逻辑设备包装器
        class VulkanDevice {
          public:
            VulkanDevice() = default;
            ~VulkanDevice();

            bool Initialize(VulkanPhysicalDevice* physicalDevice);
            void Shutdown();

            VkDevice GetHandle() const { return Device; }
            VulkanPhysicalDevice* GetPhysicalDevice() const {
                return PhysicalDevice;
            }

            VkQueue GetGraphicsQueue() const { return GraphicsQueue; }
            VkQueue GetComputeQueue() const { return ComputeQueue; }
            VkQueue GetTransferQueue() const { return TransferQueue; }

            uint32_t GetGraphicsQueueFamilyIndex() const;
            uint32_t GetComputeQueueFamilyIndex() const;
            uint32_t GetTransferQueueFamilyIndex() const;

          private:
            bool CreateLogicalDevice();
            std::vector<const char*> GetRequiredDeviceExtensions() const;

            VkDevice Device = VK_NULL_HANDLE;
            VulkanPhysicalDevice* PhysicalDevice = nullptr;

            VkQueue GraphicsQueue = VK_NULL_HANDLE;
            VkQueue ComputeQueue = VK_NULL_HANDLE;
            VkQueue TransferQueue = VK_NULL_HANDLE;
        };

        // Vulkan命令池管理器
        class VulkanCommandPoolManager {
          public:
            VulkanCommandPoolManager() = default;
            ~VulkanCommandPoolManager();

            bool Initialize(VulkanDevice* device);
            void Shutdown();

            VkCommandPool GetGraphicsCommandPool() const {
                return GraphicsCommandPool;
            }
            VkCommandPool GetComputeCommandPool() const {
                return ComputeCommandPool;
            }
            VkCommandPool GetTransferCommandPool() const {
                return TransferCommandPool;
            }

          private:
            bool CreateCommandPools();

            VulkanDevice* Device = nullptr;
            VkCommandPool GraphicsCommandPool = VK_NULL_HANDLE;
            VkCommandPool ComputeCommandPool = VK_NULL_HANDLE;
            VkCommandPool TransferCommandPool = VK_NULL_HANDLE;
        };

        // Vulkan内存分配器包装器
        class VulkanMemoryAllocator {
          public:
            VulkanMemoryAllocator() = default;
            ~VulkanMemoryAllocator();

            bool Initialize(VulkanDevice* device);
            void Shutdown();

            VkDeviceMemory AllocateMemory(
                const VkMemoryRequirements& memRequirements,
                VkMemoryPropertyFlags properties);
            void FreeMemory(VkDeviceMemory memory);

          private:
            uint32_t FindMemoryType(uint32_t typeFilter,
                                    VkMemoryPropertyFlags properties) const;

            VulkanDevice* Device = nullptr;
            std::vector<VkDeviceMemory> AllocatedMemory;
        };

        // Vulkan RHI实现类
        class VulkanRHI {
          public:
            static VulkanRHI& Get();

            bool Initialize(const RHIModuleInitParams& params);
            void Shutdown();

            VulkanInstance* GetInstance() { return &Instance; }
            VulkanDevice* GetDevice() { return &Device; }
            VulkanCommandPoolManager* GetCommandPoolManager() {
                return &CommandPoolManager;
            }
            VulkanMemoryAllocator* GetMemoryAllocator() {
                return &MemoryAllocator;
            }

          private:
            VulkanRHI() = default;
            ~VulkanRHI() = default;

            VulkanRHI(const VulkanRHI&) = delete;
            VulkanRHI& operator=(const VulkanRHI&) = delete;

            VulkanInstance Instance;
            VulkanPhysicalDevice PhysicalDevice;
            VulkanDevice Device;
            VulkanCommandPoolManager CommandPoolManager;
            VulkanMemoryAllocator MemoryAllocator;
        };

    }  // namespace RHI
}  // namespace Engine
