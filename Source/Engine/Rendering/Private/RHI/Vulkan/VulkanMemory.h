
#pragma once

#include <vulkan/vulkan.h>

#include "RHI/RHIMemory.h"
#include "VulkanRHI.h"

namespace Engine {
    namespace RHI {

        // Vulkan内存堆实现
        class VulkanHeap : public IRHIHeap {
          public:
            VulkanHeap(VkDevice device, const HeapDesc& desc);
            virtual ~VulkanHeap();

            // IRHIHeap接口实现
            virtual const HeapDesc& GetDesc() const override { return m_Desc; }
            virtual uint64 GetSize() const override {
                return m_Desc.SizeInBytes;
            }

            // IRHIResource接口实现
            virtual const std::string& GetName() const override {
                return m_Desc.DebugName;
            }
            virtual ERHIResourceState GetCurrentState() const override {
                return ERHIResourceState::Common;
            }
            virtual ERHIResourceDimension GetResourceDimension()
                const override {
                return ERHIResourceDimension::Buffer;
            }
            virtual void* Map() override;
            virtual void Unmap() override;

            // Vulkan特定方法
            VkDeviceMemory GetVkDeviceMemory() const { return m_DeviceMemory; }

          private:
            VkDevice m_Device;
            HeapDesc m_Desc;
            VkDeviceMemory m_DeviceMemory;
            void* m_MappedData;
            bool m_IsMapped;
        };

        // Vulkan内存分配器实现
        class VulkanMemoryAllocator : public IRHIMemoryAllocator {
          public:
            VulkanMemoryAllocator(VkDevice device,
                                  VkPhysicalDevice physicalDevice);
            virtual ~VulkanMemoryAllocator();

            // IRHIMemoryAllocator接口实现
            virtual IRHIHeap* CreateHeap(const HeapDesc& desc) override;
            virtual void* Allocate(uint64 size, uint64 alignment) override;
            virtual void Free(void* ptr) override;
            virtual void GetMemoryStats(uint64& totalSize,
                                        uint64& usedSize) const override;

            // Vulkan特定方法
            bool Initialize(VkDevice device);
            void Shutdown();
            VkDeviceMemory AllocateMemory(VkMemoryRequirements memRequirements,
                                          VkMemoryPropertyFlags properties);
            void FreeMemory(VkDeviceMemory memory);

          private:
            uint32_t FindMemoryType(uint32_t typeFilter,
                                    VkMemoryPropertyFlags properties);

            VkDevice m_Device;
            VkPhysicalDevice m_PhysicalDevice;
            VkPhysicalDeviceMemoryProperties m_MemoryProperties;
            std::vector<VkDeviceMemory> AllocatedMemory;

            // 内存统计
            struct {
                uint64 TotalAllocated;
                uint64 CurrentUsed;
            } m_Stats;
        };

    }  // namespace RHI
}  // namespace Engine
