#include "VulkanTimelineSemaphore.h"

#include "VulkanRHI.h"

namespace Engine {
    namespace RHI {

        VulkanTimelineSemaphore::VulkanTimelineSemaphore(
            VulkanDevice* InDevice, const TimelineSemaphoreDesc& InDesc)
            : Device(InDevice), Semaphore(VK_NULL_HANDLE), Desc(InDesc) {
            // 创建时间线信号量
            VkSemaphoreTypeCreateInfo timelineCreateInfo = {};
            timelineCreateInfo.sType =
                VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
            timelineCreateInfo.pNext = nullptr;
            timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
            timelineCreateInfo.initialValue = InDesc.InitialValue;

            VkSemaphoreCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            createInfo.pNext = &timelineCreateInfo;
            createInfo.flags = 0;

            VkResult Result = vkCreateSemaphore(
                Device->GetHandle(), &createInfo, nullptr, &Semaphore);
            if (Result != VK_SUCCESS) {
                throw std::runtime_error("Failed to create timeline semaphore");
            }

            // 设置调试名称
            if (!Desc.DebugName.empty()) {
                Device->SetDebugName(VK_OBJECT_TYPE_SEMAPHORE,
                                     (uint64)Semaphore,
                                     Desc.DebugName.c_str());
            }
        }

        VulkanTimelineSemaphore::~VulkanTimelineSemaphore() {
            if (Semaphore != VK_NULL_HANDLE) {
                vkDestroySemaphore(Device->GetHandle(), Semaphore, nullptr);
                Semaphore = VK_NULL_HANDLE;
            }
        }

        uint64 VulkanTimelineSemaphore::GetCurrentValue() const {
            uint64 value = 0;
            VkResult Result = vkGetSemaphoreCounterValue(
                Device->GetHandle(), Semaphore, &value);
            if (Result != VK_SUCCESS) {
                throw std::runtime_error(
                    "Failed to get semaphore counter value");
            }
            return value;
        }

        void VulkanTimelineSemaphore::Wait(uint64 value) {
            VkSemaphoreWaitInfo waitInfo = {};
            waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
            waitInfo.pNext = nullptr;
            waitInfo.flags = 0;
            waitInfo.semaphoreCount = 1;
            waitInfo.pSemaphores = &Semaphore;
            waitInfo.pValues = &value;

            VkResult Result =
                vkWaitSemaphores(Device->GetHandle(), &waitInfo, UINT64_MAX);
            if (Result != VK_SUCCESS) {
                throw std::runtime_error("Failed to wait on semaphore");
            }
        }

        void VulkanTimelineSemaphore::Signal(uint64 value) {
            VkSemaphoreSignalInfo signalInfo = {};
            signalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
            signalInfo.pNext = nullptr;
            signalInfo.semaphore = Semaphore;
            signalInfo.value = value;

            VkResult Result =
                vkSignalSemaphore(Device->GetHandle(), &signalInfo);
            if (Result != VK_SUCCESS) {
                throw std::runtime_error("Failed to signal semaphore");
            }
        }

    }  // namespace RHI
}  // namespace Engine
