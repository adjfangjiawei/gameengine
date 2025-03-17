
#pragma once

#include <vulkan/vulkan.h>

#include "RHI/RHISync.h"

namespace Engine {
    namespace RHI {
        class VulkanDevice;

        class VulkanEvent : public IRHIEvent {
          public:
            VulkanEvent(VulkanDevice* InDevice);
            VulkanEvent(VulkanDevice* InDevice, VkEvent InEvent);
            virtual ~VulkanEvent();

            // IRHIEvent接口实现
            virtual void Set() override;
            virtual void Reset() override;
            virtual void Wait() override;

            // Vulkan特定方法
            VkEvent GetHandle() const { return Event; }

          private:
            VulkanDevice* Device;
            VkEvent Event;
        };

    }  // namespace RHI
}  // namespace Engine
