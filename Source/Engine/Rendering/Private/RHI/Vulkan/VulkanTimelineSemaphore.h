
#pragma once

#include "RHI/RHISync.h"
#include "VulkanRHI.h"

namespace Engine {
    namespace RHI {

        class VulkanTimelineSemaphore : public IRHITimelineSemaphore {
          public:
            VulkanTimelineSemaphore(VulkanDevice* InDevice,
                                    const TimelineSemaphoreDesc& InDesc);
            VulkanTimelineSemaphore(VulkanDevice* InDevice,
                                    const TimelineSemaphoreDesc& InDesc,
                                    VkSemaphore InSemaphore);
            virtual ~VulkanTimelineSemaphore();

            // IRHITimelineSemaphore接口实现
            virtual uint64 GetCurrentValue() const override;
            virtual void Wait(uint64 value) override;
            virtual void Signal(uint64 value) override;

            // Vulkan特定方法
            VkSemaphore GetHandle() const { return Semaphore; }
            const TimelineSemaphoreDesc& GetDesc() const { return Desc; }

          private:
            VulkanDevice* Device;
            VkSemaphore Semaphore;
            TimelineSemaphoreDesc Desc;
        };

    }  // namespace RHI
}  // namespace Engine
