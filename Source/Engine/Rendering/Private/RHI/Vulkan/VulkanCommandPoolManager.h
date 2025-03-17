
#pragma once

#include <vulkan/vulkan.h>

namespace Engine {
    namespace RHI {

        class VulkanDevice;

        class VulkanCommandPoolManager {
          public:
            ~VulkanCommandPoolManager();

            bool Initialize(VulkanDevice* device);
            void Shutdown();

            // 获取命令池
            VkCommandPool GetGraphicsCommandPool() const {
                return GraphicsCommandPool;
            }
            VkCommandPool GetComputeCommandPool() const {
                return ComputeCommandPool;
            }
            VkCommandPool GetTransferCommandPool() const {
                return TransferCommandPool;
            }

            // 重置命令池
            void ResetGraphicsCommandPool();
            void ResetComputeCommandPool();
            void ResetTransferCommandPool();

            // 分配命令缓冲区
            VkCommandBuffer AllocateGraphicsCommandBuffer(
                bool isPrimary = true);
            VkCommandBuffer AllocateComputeCommandBuffer(bool isPrimary = true);
            VkCommandBuffer AllocateTransferCommandBuffer(
                bool isPrimary = true);

            // 释放命令缓冲区
            void FreeGraphicsCommandBuffer(VkCommandBuffer commandBuffer);
            void FreeComputeCommandBuffer(VkCommandBuffer commandBuffer);
            void FreeTransferCommandBuffer(VkCommandBuffer commandBuffer);

          private:
            bool CreateCommandPools();
            VkCommandBuffer AllocateCommandBuffer(VkCommandPool pool,
                                                  bool isPrimary);
            void FreeCommandBuffer(VkCommandPool pool,
                                   VkCommandBuffer commandBuffer);

          private:
            VulkanDevice* Device = nullptr;
            VkCommandPool GraphicsCommandPool = VK_NULL_HANDLE;
            VkCommandPool ComputeCommandPool = VK_NULL_HANDLE;
            VkCommandPool TransferCommandPool = VK_NULL_HANDLE;
        };

    }  // namespace RHI
}  // namespace Engine
