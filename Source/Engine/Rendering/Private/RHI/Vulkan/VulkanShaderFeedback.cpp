
#include "VulkanShaderFeedback.h"

#include "Core/Public/Assert.h"
#include "VulkanMemory.h"
#include "VulkanRHI.h"

namespace Engine {
    namespace RHI {

        VulkanShaderFeedbackBuffer::VulkanShaderFeedbackBuffer(
            VulkanDevice* InDevice,
            const ShaderFeedbackDesc& InDesc,
            VkBuffer InBuffer,
            VkDeviceMemory InMemory)
            : Device(InDevice),
              Desc(InDesc),
              Buffer(InBuffer),
              Memory(InMemory) {
            // 计算缓冲区大小
            BufferSize = InDesc.MaxEntries * sizeof(uint32);

            // 设置调试名称
            if (!Desc.DebugName.empty()) {
                Device->SetDebugName(VK_OBJECT_TYPE_BUFFER,
                                     (uint64)Buffer,
                                     Desc.DebugName.c_str());
            }
        }

        VulkanShaderFeedbackBuffer::~VulkanShaderFeedbackBuffer() {
            // Buffer和Memory由外部管理，不在此处销毁
        }

        void VulkanShaderFeedbackBuffer::GetFeedbackData(void* data,
                                                         uint32 size) {
            check(size <= BufferSize);

            void* mappedData;
            VkResult Result = vkMapMemory(
                *Device->GetHandle(), Memory, 0, size, 0, &mappedData);
            check(Result == VK_SUCCESS);

            memcpy(data, mappedData, size);
            vkUnmapMemory(*Device->GetHandle(), Memory);
        }

        void VulkanShaderFeedbackBuffer::Reset() {
            // 将缓冲区内容清零
            void* mappedData;
            VkResult Result = vkMapMemory(
                *Device->GetHandle(), Memory, 0, BufferSize, 0, &mappedData);
            check(Result == VK_SUCCESS);

            memset(mappedData, 0, BufferSize);
            vkUnmapMemory(*Device->GetHandle(), Memory);
        }

        VulkanPredicate::VulkanPredicate(VulkanDevice* InDevice,
                                         const PredicateDesc& InDesc,
                                         VkQueryPool InQueryPool)
            : Device(InDevice),
              Desc(InDesc),
              Buffer(VK_NULL_HANDLE),
              Memory(VK_NULL_HANDLE),
              QueryPool(InQueryPool) {
            // 创建谓词缓冲区
            VkBufferCreateInfo bufferInfo = {};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = sizeof(uint64);  // 足够存储所有谓词类型
            bufferInfo.usage = VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT |
                               VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VkResult Result = vkCreateBuffer(
                *Device->GetHandle(), &bufferInfo, nullptr, &Buffer);
            check(Result == VK_SUCCESS);

            // 分配并绑定内存
            VkMemoryRequirements memRequirements;
            vkGetBufferMemoryRequirements(
                *Device->GetHandle(), Buffer, &memRequirements);

            VkMemoryAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex =
                Device->GetPhysicalDevice()->GetMemoryTypeIndex(
                    memRequirements.memoryTypeBits,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            Result = vkAllocateMemory(
                *Device->GetHandle(), &allocInfo, nullptr, &Memory);
            check(Result == VK_SUCCESS);

            Result =
                vkBindBufferMemory(*Device->GetHandle(), Buffer, Memory, 0);
            check(Result == VK_SUCCESS);

            // 设置初始值
            SetValue(InDesc);

            // 设置调试名称
            if (!Desc.DebugName.empty()) {
                Device->SetDebugName(VK_OBJECT_TYPE_BUFFER,
                                     (uint64)Buffer,
                                     Desc.DebugName.c_str());
            }
        }

        VulkanPredicate::~VulkanPredicate() {
            if (QueryPool != VK_NULL_HANDLE) {
                vkDestroyQueryPool(*Device->GetHandle(), QueryPool, nullptr);
                QueryPool = VK_NULL_HANDLE;
            }
            if (Buffer != VK_NULL_HANDLE) {
                vkDestroyBuffer(*Device->GetHandle(), Buffer, nullptr);
                Buffer = VK_NULL_HANDLE;
            }
            if (Memory != VK_NULL_HANDLE) {
                vkFreeMemory(*Device->GetHandle(), Memory, nullptr);
                Memory = VK_NULL_HANDLE;
            }
        }

        void VulkanPredicate::SetValue(const PredicateDesc& value) {
            Desc = value;

            void* mappedData;
            VkResult Result = vkMapMemory(*Device->GetHandle(),
                                          Memory,
                                          0,
                                          sizeof(uint64),
                                          0,
                                          &mappedData);
            check(Result == VK_SUCCESS);

            uint64 predicateValue = 0;
            switch (value.Type) {
                case EPredicateType::Binary:
                    predicateValue =
                        value.Value.Binary.Value ? VK_TRUE : VK_FALSE;
                    break;
                case EPredicateType::Numeric:
                    predicateValue = value.Value.Numeric.Value;
                    break;
                case EPredicateType::CompareValue:
                    // 在这种情况下，我们需要根据比较操作符和引用值来设置谓词值
                    predicateValue = (value.Value.Numeric.Value >=
                                      value.Value.Numeric.Reference)
                                         ? VK_TRUE
                                         : VK_FALSE;
                    break;
            }

            memcpy(mappedData, &predicateValue, sizeof(uint64));
            vkUnmapMemory(*Device->GetHandle(), Memory);
        }

    }  // namespace RHI
}  // namespace Engine
