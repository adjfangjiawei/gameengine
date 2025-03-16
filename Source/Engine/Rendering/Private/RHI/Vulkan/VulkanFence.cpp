#include "VulkanFence.h"

namespace Engine {
    namespace RHI {

        VulkanFence::VulkanFence(VkDevice device, const FenceDesc& desc)
            : m_Device(device), m_Desc(desc) {
            if (m_Desc.Type == EFenceType::Timeline) {
                CreateTimelineSemaphore();
            } else {
                CreateFence();
            }
        }

        VulkanFence::~VulkanFence() {
            if (m_TimelineSemaphore != VK_NULL_HANDLE) {
                vkDestroySemaphore(m_Device, m_TimelineSemaphore, nullptr);
                m_TimelineSemaphore = VK_NULL_HANDLE;
            }
            if (m_Fence != VK_NULL_HANDLE) {
                vkDestroyFence(m_Device, m_Fence, nullptr);
                m_Fence = VK_NULL_HANDLE;
            }
        }

        void VulkanFence::CreateFence() {
            VkFenceCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            createInfo.flags = 0;

            // TODO: Implement proper fence sharing if needed
            if (m_Desc.Type == EFenceType::Shared) {
                // For shared fences, we might need to use
                // VkExternalFenceCreateInfo with appropriate handle types in
                // the future
            }

            VkResult result =
                vkCreateFence(m_Device, &createInfo, nullptr, &m_Fence);
            if (result != VK_SUCCESS) {
                // TODO: Error handling
            }
        }

        void VulkanFence::CreateTimelineSemaphore() {
            VkSemaphoreTypeCreateInfo timelineCreateInfo = {};
            timelineCreateInfo.sType =
                VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
            timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
            timelineCreateInfo.initialValue = m_Desc.InitialValue;

            VkSemaphoreCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            createInfo.pNext = &timelineCreateInfo;

            VkResult result = vkCreateSemaphore(
                m_Device, &createInfo, nullptr, &m_TimelineSemaphore);
            if (result != VK_SUCCESS) {
                // TODO: Error handling
            }
        }

        uint64 VulkanFence::GetCompletedValue() const {
            if (m_Desc.Type == EFenceType::Timeline) {
                uint64 value;
                vkGetSemaphoreCounterValue(
                    m_Device, m_TimelineSemaphore, &value);
                return value;
            } else {
                VkResult result = vkGetFenceStatus(m_Device, m_Fence);
                return (result == VK_SUCCESS) ? 1 : 0;
            }
        }

        void VulkanFence::Wait(uint64 value) {
            if (m_Desc.Type == EFenceType::Timeline) {
                VkSemaphoreWaitInfo waitInfo = {};
                waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
                waitInfo.semaphoreCount = 1;
                waitInfo.pSemaphores = &m_TimelineSemaphore;
                waitInfo.pValues = &value;

                vkWaitSemaphores(m_Device, &waitInfo, UINT64_MAX);
            } else {
                vkWaitForFences(m_Device, 1, &m_Fence, VK_TRUE, UINT64_MAX);
            }
        }

        void VulkanFence::Signal(uint64 value) {
            if (m_Desc.Type == EFenceType::Timeline) {
                VkSemaphoreSignalInfo signalInfo = {};
                signalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
                signalInfo.semaphore = m_TimelineSemaphore;
                signalInfo.value = value;

                vkSignalSemaphore(m_Device, &signalInfo);
            } else {
                vkResetFences(m_Device, 1, &m_Fence);
            }
        }

    }  // namespace RHI
}  // namespace Engine
