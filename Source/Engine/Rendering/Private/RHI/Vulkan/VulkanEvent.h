
#pragma once

#include "RHI/RHISync.h"
#include "VulkanRHI.h"

namespace Engine {
    namespace RHI {

        class VulkanEvent : public IRHIEvent {
          public:
            VulkanEvent(VulkanDevice* InDevice);
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
