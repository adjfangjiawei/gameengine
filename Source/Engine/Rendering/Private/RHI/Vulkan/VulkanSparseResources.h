
#pragma once

#include <vulkan/vulkan.h>

#include "RHI/RHISparseResources.h"
#include "VulkanRHI.h"

namespace Engine {
    namespace RHI {

        // Vulkan稀疏资源基类
        class VulkanSparseResource : public IRHISparseResource {
          public:
            VulkanSparseResource(const SparseResourceDesc& desc,
                                 VkDevice device);
            virtual ~VulkanSparseResource();

            // IRHISparseResource接口实现
            virtual const SparseResourceDesc& GetDesc() const override {
                return m_Desc;
            }
            virtual void GetRequiredBlocks(const SparseRegion& region,
                                           uint32& numBlocks,
                                           uint64& memorySize) const override;
            virtual void BindBlocks(uint32 numBindings,
                                    const SparseBinding* bindings) override;

          protected:
            SparseResourceDesc m_Desc;
            VkDevice m_Device;
            VkQueue m_SparseQueue;
            VkSemaphore m_SparseSemaphore;
        };

        // Vulkan稀疏纹理实现
        class VulkanSparseTexture : public IRHISparseTexture,
                                    public VulkanSparseResource {
          public:
            VulkanSparseTexture(const SparseResourceDesc& desc,
                                EPixelFormat format,
                                ERHIResourceDimension dimension,
                                VkDevice device);
            virtual ~VulkanSparseTexture();

            // IRHISparseTexture接口实现
            virtual EPixelFormat GetFormat() const override { return m_Format; }
            virtual ERHIResourceDimension GetDimension() const override {
                return m_Dimension;
            }

          private:
            EPixelFormat m_Format;
            ERHIResourceDimension m_Dimension;
            VkImage m_Image;
            VkImageView m_ImageView;
        };

        // Vulkan稀疏缓冲区实现
        class VulkanSparseBuffer : public IRHISparseBuffer,
                                   public VulkanSparseResource {
          public:
            VulkanSparseBuffer(const SparseResourceDesc& desc,
                               uint64 size,
                               uint32 stride,
                               VkDevice device);
            virtual ~VulkanSparseBuffer();

            // IRHISparseBuffer接口实现
            virtual uint64 GetSize() const override { return m_Size; }
            virtual uint32 GetStride() const override { return m_Stride; }

          private:
            uint64 m_Size;
            uint32 m_Stride;
            VkBuffer m_Buffer;
        };

    }  // namespace RHI
}  // namespace Engine
