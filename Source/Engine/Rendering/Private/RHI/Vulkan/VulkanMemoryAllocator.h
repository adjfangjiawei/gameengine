
#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace Engine {
    namespace RHI {

        class VulkanMemoryAllocator {
          public:
            struct MemoryStats {
                uint64_t TotalAllocated;  // 总共分配的内存
                uint64_t CurrentUsed;     // 当前使用的内存

                MemoryStats() : TotalAllocated(0), CurrentUsed(0) {}
            };

            VulkanMemoryAllocator(VkDevice device,
                                  VkPhysicalDevice physicalDevice);
            ~VulkanMemoryAllocator();

            bool Initialize(VkDevice device);
            void Shutdown();

            // 内存分配和释放
            VkDeviceMemory AllocateMemory(VkMemoryRequirements memRequirements,
                                          VkMemoryPropertyFlags properties);
            void FreeMemory(VkDeviceMemory memory);

            // 内存类型查找
            uint32_t FindMemoryType(uint32_t typeFilter,
                                    VkMemoryPropertyFlags properties) const;

            // 获取统计信息
            const MemoryStats& GetStats() const { return m_Stats; }

          private:
            VkDevice m_Device = VK_NULL_HANDLE;
            VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
            VkPhysicalDeviceMemoryProperties m_MemoryProperties;
            std::vector<VkDeviceMemory> AllocatedMemory;
            MemoryStats m_Stats;
        };

    }  // namespace RHI
}  // namespace Engine
