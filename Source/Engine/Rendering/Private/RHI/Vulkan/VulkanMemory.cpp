
#include "VulkanMemory.h"

#include <cassert>

namespace Engine {
    namespace RHI {

        namespace {
            // 将RHI内存类型转换为Vulkan内存属性
            VkMemoryPropertyFlags GetVkMemoryProperties(EMemoryType type) {
                switch (type) {
                    case EMemoryType::Default:
                        return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                    case EMemoryType::Upload:
                        return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
                    case EMemoryType::Readback:
                        return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                               VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
                    default:
                        return 0;
                }
            }
        }  // namespace

        // VulkanHeap实现
        VulkanHeap::VulkanHeap(VkDevice device, const HeapDesc& desc)
            : m_Device(device),
              m_Desc(desc),
              m_DeviceMemory(VK_NULL_HANDLE),
              m_MappedData(nullptr),
              m_IsMapped(false) {
            VkMemoryAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = desc.SizeInBytes;

            // 根据堆类型设置内存属性
            VkMemoryPropertyFlags memProperties = GetVkMemoryProperties(
                static_cast<EMemoryType>(desc.Properties.Type));

            // 找到合适的内存类型
            VkPhysicalDeviceMemoryProperties physMemProperties;
            VulkanDevice* vulkanDevice =
                static_cast<VulkanDevice*>(VulkanRHI::Get().GetDevice());
            vkGetPhysicalDeviceMemoryProperties(
                vulkanDevice->GetPhysicalDevice()->GetHandle(),
                &physMemProperties);

            // 查找满足要求的内存类型索引
            uint32_t typeIndex = UINT32_MAX;
            for (uint32_t i = 0; i < physMemProperties.memoryTypeCount; i++) {
                if ((physMemProperties.memoryTypes[i].propertyFlags &
                     memProperties) == memProperties) {
                    typeIndex = i;
                    break;
                }
            }
            assert(typeIndex != UINT32_MAX &&
                   "Failed to find suitable memory type");

            allocInfo.memoryTypeIndex = typeIndex;

            // 分配设备内存
            VkResult result = vkAllocateMemory(
                m_Device, &allocInfo, nullptr, &m_DeviceMemory);
            assert(result == VK_SUCCESS && "Failed to allocate device memory");
        }

        VulkanHeap::~VulkanHeap() {
            if (m_IsMapped) {
                vkUnmapMemory(m_Device, m_DeviceMemory);
                m_MappedData = nullptr;
                m_IsMapped = false;
            }

            if (m_DeviceMemory != VK_NULL_HANDLE) {
                vkFreeMemory(m_Device, m_DeviceMemory, nullptr);
                m_DeviceMemory = VK_NULL_HANDLE;
            }
        }

        void* VulkanHeap::Map() {
            if (!m_IsMapped) {
                VkResult result = vkMapMemory(m_Device,
                                              m_DeviceMemory,
                                              0,
                                              m_Desc.SizeInBytes,
                                              0,
                                              &m_MappedData);
                assert(result == VK_SUCCESS && "Failed to map memory");
                m_IsMapped = true;
            }
            return m_MappedData;
        }

        void VulkanHeap::Unmap() {
            if (m_IsMapped) {
                vkUnmapMemory(m_Device, m_DeviceMemory);
                m_MappedData = nullptr;
                m_IsMapped = false;
            }
        }

        // VulkanMemoryAllocator实现
        VulkanMemoryAllocator::VulkanMemoryAllocator(
            VkDevice device, VkPhysicalDevice physicalDevice)
            : m_Device(device),
              m_PhysicalDevice(physicalDevice),
              m_Stats{0, 0} {
            vkGetPhysicalDeviceMemoryProperties(physicalDevice,
                                                &m_MemoryProperties);
        }

        VulkanMemoryAllocator::~VulkanMemoryAllocator() {
            // 确保所有分配的内存都已被释放
            assert(m_Stats.CurrentUsed == 0 &&
                   "Memory leak detected in VulkanMemoryAllocator");
        }

        IRHIHeap* VulkanMemoryAllocator::CreateHeap(const HeapDesc& desc) {
            VulkanHeap* heap = new VulkanHeap(m_Device, desc);
            m_Stats.TotalAllocated += desc.SizeInBytes;
            m_Stats.CurrentUsed += desc.SizeInBytes;
            return heap;
        }

        void* VulkanMemoryAllocator::Allocate(uint64 size, uint64 alignment) {
            // 根据对齐要求调整大小
            uint64 alignedSize = (size + alignment - 1) & ~(alignment - 1);

            // 创建一个临时堆来分配内存
            HeapDesc desc;
            desc.SizeInBytes = alignedSize;            // 使用对齐后的大小
            desc.Properties.Type = EHeapType::Upload;  // 默认使用上传堆类型

            VulkanHeap* heap = static_cast<VulkanHeap*>(CreateHeap(desc));
            return heap->Map();
        }

        void VulkanMemoryAllocator::Free(void* ptr) {
            // 在实际实现中，我们需要跟踪每个分配的内存块
            // 这里简化处理，实际项目中应该使用内存池或其他更复杂的内存管理策略
            if (ptr) {
                // TODO: 实现proper内存释放
                m_Stats.CurrentUsed -= 0;  // 更新统计信息
            }
        }

        void VulkanMemoryAllocator::GetMemoryStats(uint64& totalSize,
                                                   uint64& usedSize) const {
            totalSize = m_Stats.TotalAllocated;
            usedSize = m_Stats.CurrentUsed;
        }

        uint32 VulkanMemoryAllocator::FindMemoryType(
            uint32 typeFilter, VkMemoryPropertyFlags properties) {
            for (uint32 i = 0; i < m_MemoryProperties.memoryTypeCount; i++) {
                if ((typeFilter & (1 << i)) &&
                    (m_MemoryProperties.memoryTypes[i].propertyFlags &
                     properties) == properties) {
                    return i;
                }
            }

            assert(false && "Failed to find suitable memory type");
            return 0;
        }

    }  // namespace RHI
}  // namespace Engine
