#pragma once

#include <xcb/xcb.h>

#include "VulkanRHI.h"

namespace Engine {
    namespace RHI {

        // 扩展VulkanPhysicalDevice的QueueFamilyIndices结构
        struct VulkanQueueFamilyIndices {
            uint32_t GraphicsFamily;
            uint32_t ComputeFamily;
            uint32_t TransferFamily;
            uint32_t PresentFamily;

            bool IsComplete() const {
                return GraphicsFamily != UINT32_MAX &&
                       ComputeFamily != UINT32_MAX &&
                       TransferFamily != UINT32_MAX &&
                       PresentFamily != UINT32_MAX;
            }
        };

        // 扩展VulkanDevice类的功能
        class VulkanDevicePresent {
          public:
            static uint32_t GetPresentQueueFamilyIndex(VulkanDevice* device) {
                return device
                    ->GetGraphicsQueueFamilyIndex();  // 通常Present队列与Graphics队列相同
            }

            static VkQueue GetPresentQueue(VulkanDevice* device) {
                return device
                    ->GetGraphicsQueue();  // 通常Present队列与Graphics队列相同
            }
        };

    }  // namespace RHI
}  // namespace Engine
