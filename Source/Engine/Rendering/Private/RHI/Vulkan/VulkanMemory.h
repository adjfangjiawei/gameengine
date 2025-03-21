#pragma once

#include <vulkan/vulkan.h>

#include <mutex>
#include <unordered_map>

#include "RHI/RHIMemory.h"
// Forward declarations
namespace Engine {
    namespace RHI {
        class VulkanDevice;
    }
}  // namespace Engine

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
            virtual void* GetNativeHandle() const override {
                return (void*)(uint64_t)m_DeviceMemory;
            }

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
            bool Initialize(const VkDevice* device);
            void Shutdown();
            VkDeviceMemory AllocateMemory(VkMemoryRequirements memRequirements,
                                          VkMemoryPropertyFlags properties);
            void FreeMemory(VkDeviceMemory memory);

            // 获取内存统计信息
            struct MemoryStats {
                uint64 TotalAllocated;
                uint64 CurrentUsed;
            };
            const MemoryStats& GetStats() const { return m_Stats; }

          private:
            struct AllocationInfo {
                VkDeviceMemory Memory;
                uint64 Size;
                bool IsMapped;
                uint32 MemoryTypeIndex;
            };

            uint32_t FindMemoryType(uint32_t typeFilter,
                                    VkMemoryPropertyFlags properties);
            void DefragmentMemory();

            static constexpr size_t MAX_ALLOCATION_COUNT = 1000;
            static constexpr float FRAGMENTATION_THRESHOLD = 0.7f;

            VkDevice m_Device;
            VkPhysicalDevice m_PhysicalDevice;
            VkPhysicalDeviceMemoryProperties m_MemoryProperties;
            std::vector<VkDeviceMemory> AllocatedMemory;
            std::mutex m_AllocationMutex;
            std::unordered_map<void*, AllocationInfo> m_Allocations;

            MemoryStats m_Stats;
        };

    }  // namespace RHI
}  // namespace Engine
