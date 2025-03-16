
#include "VulkanSparseResources.h"

#include "Core/Public/Log/LogSystem.h"
#include "VulkanRHI.h"

namespace Engine {
    namespace RHI {

        // VulkanSparseResource实现
        VulkanSparseResource::VulkanSparseResource(
            const SparseResourceDesc& desc, VkDevice device)
            : m_Desc(desc),
              m_Device(device),
              m_SparseQueue(VK_NULL_HANDLE),
              m_SparseSemaphore(VK_NULL_HANDLE) {
            // 创建用于稀疏绑定的信号量
            VkSemaphoreCreateInfo semaphoreInfo = {};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            VkResult result = vkCreateSemaphore(
                m_Device, &semaphoreInfo, nullptr, &m_SparseSemaphore);
            if (result != VK_SUCCESS) {
                LOG_ERROR("Failed to create sparse binding semaphore");
            }

            // 获取支持稀疏绑定的队列
            // Note: 这里需要从设备中获取支持稀疏绑定的队列族索引
            // 并从中获取一个队列用于稀疏内存绑定操作
        }

        VulkanSparseResource::~VulkanSparseResource() {
            if (m_SparseSemaphore != VK_NULL_HANDLE) {
                vkDestroySemaphore(m_Device, m_SparseSemaphore, nullptr);
            }
        }

        void VulkanSparseResource::GetRequiredBlocks(const SparseRegion& region,
                                                     uint32& numBlocks,
                                                     uint64& memorySize) const {
            // 计算给定区域需要的块数量
            uint32 blockWidth =
                (region.ExtentWidth + m_Desc.BlockSize.Width - 1) /
                m_Desc.BlockSize.Width;
            uint32 blockHeight =
                (region.ExtentHeight + m_Desc.BlockSize.Height - 1) /
                m_Desc.BlockSize.Height;
            uint32 blockDepth =
                (region.ExtentDepth + m_Desc.BlockSize.Depth - 1) /
                m_Desc.BlockSize.Depth;

            numBlocks = blockWidth * blockHeight * blockDepth;
            memorySize = numBlocks * m_Desc.BlockSize.Width *
                         m_Desc.BlockSize.Height * m_Desc.BlockSize.Depth;
        }

        void VulkanSparseResource::BindBlocks(uint32 numBindings,
                                              const SparseBinding* bindings) {
            if (!m_SparseQueue) {
                LOG_ERROR("No sparse binding queue available");
                return;
            }

            std::vector<VkSparseMemoryBind> memoryBinds;
            memoryBinds.reserve(numBindings);

            for (uint32 i = 0; i < numBindings; i++) {
                const auto& binding = bindings[i];

                VkSparseMemoryBind memoryBind = {};
                memoryBind.resourceOffset = binding.MemoryOffset;
                memoryBind.size = binding.Region.ExtentWidth *
                                  binding.Region.ExtentHeight *
                                  binding.Region.ExtentDepth;
                auto vulkanHeap = static_cast<VulkanHeap*>(binding.Memory);
                memoryBind.memory = vulkanHeap ? vulkanHeap->GetVkDeviceMemory()
                                               : VK_NULL_HANDLE;
                memoryBind.memoryOffset = binding.MemoryOffset;
                memoryBind.flags = 0;

                memoryBinds.push_back(memoryBind);
            }

            VkBindSparseInfo bindInfo = {};
            bindInfo.sType = VK_STRUCTURE_TYPE_BIND_SPARSE_INFO;
            bindInfo.waitSemaphoreCount = 0;
            bindInfo.signalSemaphoreCount = 1;
            bindInfo.pSignalSemaphores = &m_SparseSemaphore;

            // 根据资源类型设置具体的绑定信息
            // 这部分在子类中实现

            VkResult result =
                vkQueueBindSparse(m_SparseQueue, 1, &bindInfo, VK_NULL_HANDLE);
            if (result != VK_SUCCESS) {
                LOG_ERROR("Failed to bind sparse memory blocks");
            }
        }

        // VulkanSparseTexture实现
        VulkanSparseTexture::VulkanSparseTexture(
            const SparseResourceDesc& desc,
            EPixelFormat format,
            ERHIResourceDimension dimension,
            VkDevice device)
            : VulkanSparseResource(desc, device),
              m_Format(format),
              m_Dimension(dimension),
              m_Image(VK_NULL_HANDLE),
              m_ImageView(VK_NULL_HANDLE) {
            VkImageCreateInfo imageInfo = {};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.flags = VK_IMAGE_CREATE_SPARSE_BINDING_BIT |
                              VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT;

            switch (dimension) {
                case ERHIResourceDimension::Texture2D:
                    imageInfo.imageType = VK_IMAGE_TYPE_2D;
                    break;
                case ERHIResourceDimension::Texture3D:
                    imageInfo.imageType = VK_IMAGE_TYPE_3D;
                    break;
                default:
                    LOG_ERROR("Unsupported sparse texture dimension");
                    return;
            }

            // 设置图像格式和其他属性
            imageInfo.format = VulkanDevice::ConvertToVkFormat(format);
            imageInfo.extent.width = desc.BlockSize.Width;
            imageInfo.extent.height = desc.BlockSize.Height;
            imageInfo.extent.depth = desc.BlockSize.Depth;
            imageInfo.mipLevels = desc.MipLevels;
            imageInfo.arrayLayers = desc.ArraySize;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VkResult result =
                vkCreateImage(m_Device, &imageInfo, nullptr, &m_Image);
            if (result != VK_SUCCESS) {
                LOG_ERROR("Failed to create sparse image");
                return;
            }

            // 创建图像视图
            VkImageViewCreateInfo viewInfo = {};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = m_Image;
            viewInfo.viewType = dimension == ERHIResourceDimension::Texture2D
                                    ? VK_IMAGE_VIEW_TYPE_2D
                                    : VK_IMAGE_VIEW_TYPE_3D;
            viewInfo.format = imageInfo.format;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = desc.MipLevels;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = desc.ArraySize;

            result =
                vkCreateImageView(m_Device, &viewInfo, nullptr, &m_ImageView);
            if (result != VK_SUCCESS) {
                LOG_ERROR("Failed to create sparse image view");
            }
        }

        VulkanSparseTexture::~VulkanSparseTexture() {
            if (m_ImageView != VK_NULL_HANDLE) {
                vkDestroyImageView(m_Device, m_ImageView, nullptr);
            }
            if (m_Image != VK_NULL_HANDLE) {
                vkDestroyImage(m_Device, m_Image, nullptr);
            }
        }

        // VulkanSparseBuffer实现
        VulkanSparseBuffer::VulkanSparseBuffer(const SparseResourceDesc& desc,
                                               uint64 size,
                                               uint32 stride,
                                               VkDevice device)
            : VulkanSparseResource(desc, device),
              m_Size(size),
              m_Stride(stride),
              m_Buffer(VK_NULL_HANDLE) {
            VkBufferCreateInfo bufferInfo = {};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.flags = VK_BUFFER_CREATE_SPARSE_BINDING_BIT |
                               VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT;
            bufferInfo.size = size;
            bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VkResult result =
                vkCreateBuffer(m_Device, &bufferInfo, nullptr, &m_Buffer);
            if (result != VK_SUCCESS) {
                LOG_ERROR("Failed to create sparse buffer");
            }
        }

        VulkanSparseBuffer::~VulkanSparseBuffer() {
            if (m_Buffer != VK_NULL_HANDLE) {
                vkDestroyBuffer(m_Device, m_Buffer, nullptr);
            }
        }

    }  // namespace RHI
}  // namespace Engine
