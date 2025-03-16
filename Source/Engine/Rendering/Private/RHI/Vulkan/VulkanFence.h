#pragma once

#include <vulkan/vulkan.h>

#include "RHI/RHICommands.h"
#include "VulkanRHI.h"

namespace Engine {
    namespace RHI {

        class VulkanFence : public IRHIFence {
          public:
            VulkanFence(VkDevice device, const FenceDesc& desc);
            virtual ~VulkanFence();

            // IRHIFence interface
            virtual const FenceDesc& GetDesc() const override { return m_Desc; }
            virtual uint64 GetCompletedValue() const override;
            virtual void Wait(uint64 value) override;
            virtual void Signal(uint64 value) override;

            // Vulkan specific
            VkFence GetVkFence() const { return m_Fence; }
            VkSemaphore GetVkTimelineSemaphore() const {
                return m_TimelineSemaphore;
            }

          private:
            void CreateFence();
            void CreateTimelineSemaphore();

            VkDevice m_Device;
            FenceDesc m_Desc;
            VkFence m_Fence = VK_NULL_HANDLE;
            VkSemaphore m_TimelineSemaphore = VK_NULL_HANDLE;
        };

    }  // namespace RHI
}  // namespace Engine
