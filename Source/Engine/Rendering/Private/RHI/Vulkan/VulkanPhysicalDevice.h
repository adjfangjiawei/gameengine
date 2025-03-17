
#pragma once

#include <vulkan/vulkan.h>

#include <memory>

#include "RHI/RHIModule.h"
#include "VulkanMemory.h"

namespace Engine {
    namespace RHI {

        class VulkanPhysicalDevice {
          public:
            struct QueueFamilyIndices {
                uint32_t GraphicsFamily;
                uint32_t ComputeFamily;
                uint32_t TransferFamily;

                bool IsComplete() const {
                    return GraphicsFamily != UINT32_MAX &&
                           ComputeFamily != UINT32_MAX &&
                           TransferFamily != UINT32_MAX;
                }

                QueueFamilyIndices() {
                    GraphicsFamily = UINT32_MAX;
                    ComputeFamily = UINT32_MAX;
                    TransferFamily = UINT32_MAX;
                }
            };

            bool Initialize(VkInstance instance);
            VkPhysicalDevice GetHandle() const { return PhysicalDevice; }
            const QueueFamilyIndices& GetQueueFamilyIndices() const {
                return QueueFamilies;
            }
            VulkanMemoryAllocator* GetMemoryAllocator() const {
                return MemoryAllocator.get();
            }
            ERHIFeatureLevel GetFeatureLevel() const { return FeatureLevel; }
            uint32_t GetMemoryTypeIndex(uint32_t typeFilter,
                                        VkMemoryPropertyFlags properties) const;
            VkInstance GetInstance() const { return Instance; }

          private:
            bool SelectPhysicalDevice(VkInstance instance);
            int RateDeviceSuitability(VkPhysicalDevice device) const;
            QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;
            ERHIFeatureLevel DetermineFeatureLevel(
                const VkPhysicalDeviceProperties& props) const;

          private:
            VkInstance Instance = VK_NULL_HANDLE;
            VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
            VkPhysicalDeviceProperties Properties = {};
            VkPhysicalDeviceFeatures Features = {};
            QueueFamilyIndices QueueFamilies;
            ERHIFeatureLevel FeatureLevel = ERHIFeatureLevel::ES3_1;
            std::unique_ptr<VulkanMemoryAllocator> MemoryAllocator;
        };

    }  // namespace RHI
}  // namespace Engine
