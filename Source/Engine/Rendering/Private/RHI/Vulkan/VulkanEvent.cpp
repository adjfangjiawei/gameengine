#include "VulkanEvent.h"

#include "Core/Public/Assert.h"
#include "VulkanRHI.h"

namespace Engine {
    namespace RHI {

        VulkanEvent::VulkanEvent(VulkanDevice* InDevice)
            : Device(InDevice), Event(VK_NULL_HANDLE) {
            VkEventCreateInfo eventInfo = {};
            eventInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
            eventInfo.flags = 0;

            VkResult Result =
                vkCreateEvent(Device->GetHandle(), &eventInfo, nullptr, &Event);
            check(Result == VK_SUCCESS);
        }

        VulkanEvent::~VulkanEvent() {
            if (Event != VK_NULL_HANDLE) {
                vkDestroyEvent(Device->GetHandle(), Event, nullptr);
                Event = VK_NULL_HANDLE;
            }
        }

        void VulkanEvent::Set() {
            VkResult Result = vkSetEvent(Device->GetHandle(), Event);
            check(Result == VK_SUCCESS);
        }

        void VulkanEvent::Reset() {
            VkResult Result = vkResetEvent(Device->GetHandle(), Event);
            check(Result == VK_SUCCESS);
        }

        void VulkanEvent::Wait() {
            VkResult Result;
            do {
                Result = vkGetEventStatus(Device->GetHandle(), Event);
            } while (Result != VK_EVENT_SET);
        }

    }  // namespace RHI
}  // namespace Engine
