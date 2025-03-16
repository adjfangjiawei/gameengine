#pragma once

#include <string>

#include "VulkanRHI.h"

namespace Engine {
    namespace RHI {
        // 将RHI资源状态转换为Vulkan访问标志和布局
        void ConvertResourceState(ERHIResourceState state,
                                  VkAccessFlags& accessFlags,
                                  VkImageLayout& imageLayout);

        // Vulkan资源基类
        class VulkanResource : public IRHIResource {
          public:
            VulkanResource(VulkanDevice* device, const std::string& debugName);
            virtual ~VulkanResource() = default;

            // IRHIResource接口实现
            virtual const std::string& GetName() const override {
                return DebugName;
            }
            virtual ERHIResourceState GetCurrentState() const override {
                return CurrentState;
            }
            virtual ERHIResourceDimension GetResourceDimension()
                const override = 0;
            virtual uint64 GetSize() const override = 0;

          protected:
            VulkanDevice* Device;
            std::string DebugName;
            ERHIResourceState CurrentState;
        };

        // Vulkan缓冲区实现
        class VulkanBuffer : public VulkanResource, public IRHIBuffer {
          public:
            explicit VulkanBuffer(VulkanDevice* device, const BufferDesc& desc);
            void SetHandle(VkBuffer buffer, VkDeviceMemory memory);
            virtual ~VulkanBuffer();

            // IRHIBuffer接口实现
            virtual const BufferDesc& GetDesc() const override { return Desc; }
            virtual void* Map(uint32 subresource = 0) override;
            virtual void Unmap(uint32 subresource = 0) override;
            virtual uint64 GetGPUVirtualAddress() const override;

            virtual const std::string& GetName() const override {
                return DebugName;
            }
            virtual ERHIResourceState GetCurrentState() const override {
                return CurrentState;
            }

            // IRHIResource接口实现
            virtual ERHIResourceDimension GetResourceDimension()
                const override {
                return ERHIResourceDimension::Buffer;
            }
            virtual uint64 GetSize() const override { return Desc.SizeInBytes; }

            // Vulkan特定方法
            VkBuffer GetHandle() const { return Buffer; }
            VkDeviceMemory GetMemory() const { return Memory; }

          private:
            bool CreateBuffer();
            bool AllocateMemory();

            BufferDesc Desc;
            VkBuffer Buffer;
            VkDeviceMemory Memory;
            void* MappedData;
        };

        // Vulkan纹理实现
        class VulkanTexture : public VulkanResource, public IRHITexture {
          public:
            explicit VulkanTexture(VulkanDevice* device,
                                   const TextureDesc& desc);
            void SetHandle(VkImage image,
                           VkDeviceMemory memory,
                           VkImageView view);
            virtual ~VulkanTexture();

            // IRHITexture接口实现
            virtual const TextureDesc& GetDesc() const override { return Desc; }
            virtual EPixelFormat GetFormat() const override {
                return Desc.Format;
            }
            virtual uint32 GetWidth() const override { return Desc.Width; }
            virtual uint32 GetHeight() const override { return Desc.Height; }
            virtual uint32 GetDepth() const override { return Desc.Depth; }
            virtual uint32 GetMipLevels() const override {
                return Desc.MipLevels;
            }
            virtual uint32 GetArraySize() const override {
                return Desc.ArraySize;
            }

            // IRHIResource接口实现
            virtual ERHIResourceDimension GetResourceDimension()
                const override {
                return Desc.Dimension;
            }
            virtual uint64 GetSize() const override;

            virtual const std::string& GetName() const override {
                return DebugName;
            }
            virtual ERHIResourceState GetCurrentState() const override {
                return CurrentState;
            }

            // Vulkan特定方法
            VkImage GetHandle() const { return Image; }
            VkDeviceMemory GetMemory() const { return Memory; }
            VkImageView GetDefaultView() const { return DefaultView; }

          private:
            bool CreateImage();
            bool AllocateMemory();
            bool CreateDefaultView();
            VkFormat GetVulkanFormat() const;

            TextureDesc Desc;
            VkImage Image;
            VkDeviceMemory Memory;
            VkImageView DefaultView;
        };

        // Vulkan着色器实现
        class VulkanShader : public VulkanResource, public IRHIShader {
          public:
            explicit VulkanShader(VulkanDevice* device, const ShaderDesc& desc);
            void SetHandle(VkShaderModule shaderModule,
                           VkPipelineShaderStageCreateInfo stageInfo);
            virtual ~VulkanShader();

            // IRHIShader接口实现
            virtual const ShaderDesc& GetDesc() const override { return Desc; }
            virtual EShaderType GetType() const override { return Desc.Type; }
            virtual const std::string& GetEntryPoint() const override {
                return Desc.EntryPoint;
            }
            virtual const std::string& GetName() const override {
                return DebugName;
            }
            virtual ERHIResourceState GetCurrentState() const override {
                return CurrentState;
            }
            // IRHIResource接口实现
            virtual ERHIResourceDimension GetResourceDimension()
                const override {
                return ERHIResourceDimension::Unknown;
            }
            virtual uint64 GetSize() const override { return ShaderSize; }

            // Vulkan特定方法
            VkShaderModule GetHandle() const { return ShaderModule; }

          private:
            bool CreateShaderModule(const void* shaderData, size_t dataSize);

            ShaderDesc Desc;
            VkShaderModule ShaderModule;
            VkPipelineShaderStageCreateInfo ShaderStageInfo;
            size_t ShaderSize;
        };

        // Vulkan渲染目标视图实现
        class VulkanRenderTargetView : public VulkanResource,
                                       public IRHIRenderTargetView {
          public:
            explicit VulkanRenderTargetView(VulkanDevice* device,
                                            const RenderTargetViewDesc& desc);
            void SetHandle(VkImageView imageView,
                           VkDescriptorImageInfo descriptorInfo);
            void SetResource(IRHIResource* resource);
            virtual ~VulkanRenderTargetView();

            // IRHIRenderTargetView接口实现
            virtual const RenderTargetViewDesc& GetDesc() const override {
                return Desc;
            }
            virtual IRHIResource* GetResource() const override {
                return Resource;
            }
            virtual const std::string& GetName() const override {
                return DebugName;
            }
            virtual ERHIResourceState GetCurrentState() const override {
                return CurrentState;
            }

            // IRHIResource接口实现
            virtual ERHIResourceDimension GetResourceDimension()
                const override {
                return Resource->GetResourceDimension();
            }
            virtual uint64 GetSize() const override { return 0; }

            // Vulkan特定方法
            VkImageView GetHandle() const { return ImageView; }

          private:
            bool CreateImageView();

            RenderTargetViewDesc Desc;
            IRHIResource* Resource;
            VkImageView ImageView;
            VkDescriptorImageInfo DescriptorInfo;
        };

        // Vulkan深度模板视图实现
        class VulkanDepthStencilView : public VulkanResource,
                                       public IRHIDepthStencilView {
          public:
            explicit VulkanDepthStencilView(VulkanDevice* device,
                                            const DepthStencilViewDesc& desc);
            void SetHandle(VkImageView imageView,
                           VkDescriptorImageInfo descriptorInfo);
            void SetResource(IRHIResource* resource);
            virtual ~VulkanDepthStencilView();

            // IRHIDepthStencilView接口实现
            virtual const DepthStencilViewDesc& GetDesc() const override {
                return Desc;
            }
            virtual IRHIResource* GetResource() const override {
                return Resource;
            }

            virtual const std::string& GetName() const override {
                return DebugName;
            }
            virtual ERHIResourceState GetCurrentState() const override {
                return CurrentState;
            }
            // IRHIResource接口实现
            virtual ERHIResourceDimension GetResourceDimension()
                const override {
                return Resource->GetResourceDimension();
            }
            virtual uint64 GetSize() const override { return 0; }

            // Vulkan特定方法
            VkImageView GetHandle() const { return ImageView; }

          private:
            bool CreateImageView();

            DepthStencilViewDesc Desc;
            IRHIResource* Resource;
            VkImageView ImageView;
            VkDescriptorImageInfo DescriptorInfo;
        };

        // Vulkan着色器资源视图实现
        class VulkanShaderResourceView : public VulkanResource,
                                         public IRHIShaderResourceView {
          public:
            VulkanShaderResourceView(VulkanDevice* device,
                                     IRHIResource* resource,
                                     const ShaderResourceViewDesc& desc);
            virtual ~VulkanShaderResourceView();

            // IRHIShaderResourceView接口实现
            virtual const ShaderResourceViewDesc& GetDesc() const override {
                return Desc;
            }
            virtual IRHIResource* GetResource() const override {
                return Resource;
            }

            virtual const std::string& GetName() const override {
                return DebugName;
            }
            virtual ERHIResourceState GetCurrentState() const override {
                return CurrentState;
            }

            // IRHIResource接口实现
            virtual ERHIResourceDimension GetResourceDimension()
                const override {
                return Resource->GetResourceDimension();
            }
            virtual uint64 GetSize() const override { return 0; }

            // Vulkan特定方法
            VkImageView GetImageView() const { return ImageView; }
            VkBufferView GetBufferView() const { return BufferView; }

          private:
            bool CreateView();

            ShaderResourceViewDesc Desc;
            IRHIResource* Resource;
            VkImageView ImageView;
            VkBufferView BufferView;
        };

        // Vulkan无序访问视图实现
        class VulkanUnorderedAccessView : public VulkanResource,
                                          public IRHIUnorderedAccessView {
          public:
            VulkanUnorderedAccessView(VulkanDevice* device,
                                      IRHIResource* resource,
                                      const UnorderedAccessViewDesc& desc);
            virtual ~VulkanUnorderedAccessView();

            // IRHIUnorderedAccessView接口实现
            virtual const UnorderedAccessViewDesc& GetDesc() const override {
                return Desc;
            }
            virtual IRHIResource* GetResource() const override {
                return Resource;
            }

            // IRHIResource接口实现
            virtual ERHIResourceDimension GetResourceDimension()
                const override {
                return Resource->GetResourceDimension();
            }
            virtual uint64 GetSize() const override { return 0; }

            // Vulkan特定方法
            VkImageView GetImageView() const { return ImageView; }
            VkBufferView GetBufferView() const { return BufferView; }

          private:
            bool CreateView();

            UnorderedAccessViewDesc Desc;
            IRHIResource* Resource;
            VkImageView ImageView;
            VkBufferView BufferView;
        };

    }  // namespace RHI
}  // namespace Engine
