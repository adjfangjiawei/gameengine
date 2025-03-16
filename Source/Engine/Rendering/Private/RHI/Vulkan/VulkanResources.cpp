
#include "VulkanResources.h"

#include "Core/Public/Log/LogSystem.h"

namespace Engine {
    namespace RHI {

        namespace {
            // 将RHI格式转换为Vulkan格式
            VkFormat ConvertToVulkanFormat(EPixelFormat format) {
                switch (format) {
                    case EPixelFormat::R8G8B8A8_UNORM:
                        return VK_FORMAT_R8G8B8A8_UNORM;
                    case EPixelFormat::B8G8R8A8_UNORM:
                        return VK_FORMAT_B8G8R8A8_UNORM;
                    case EPixelFormat::R32G32B32A32_FLOAT:
                        return VK_FORMAT_R32G32B32A32_SFLOAT;
                    case EPixelFormat::R32G32B32_FLOAT:
                        return VK_FORMAT_R32G32B32_SFLOAT;
                    case EPixelFormat::R32G32_FLOAT:
                        return VK_FORMAT_R32G32_SFLOAT;
                    case EPixelFormat::R32_FLOAT:
                        return VK_FORMAT_R32_SFLOAT;
                    case EPixelFormat::D24_UNORM_S8_UINT:
                        return VK_FORMAT_D24_UNORM_S8_UINT;
                    case EPixelFormat::D32_FLOAT:
                        return VK_FORMAT_D32_SFLOAT;
                    case EPixelFormat::D32_FLOAT_S8X24_UINT:
                        return VK_FORMAT_D32_SFLOAT_S8_UINT;
                    default:
                        return VK_FORMAT_UNDEFINED;
                }
            }

        }  // namespace

        // 将RHI资源状态转换为Vulkan访问标志和布局
        void ConvertResourceState(ERHIResourceState state,
                                  VkAccessFlags& accessFlags,
                                  VkImageLayout& imageLayout) {
            switch (state) {
                case ERHIResourceState::Common:
                    accessFlags = 0;
                    imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                    break;
                case ERHIResourceState::VertexBuffer:
                    accessFlags = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
                    imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    break;
                case ERHIResourceState::ConstantBuffer:
                    accessFlags = VK_ACCESS_UNIFORM_READ_BIT;
                    imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    break;
                case ERHIResourceState::IndexBuffer:
                    accessFlags = VK_ACCESS_INDEX_READ_BIT;
                    imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    break;
                case ERHIResourceState::RenderTarget:
                    accessFlags = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                    imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    break;
                case ERHIResourceState::UnorderedAccess:
                    accessFlags =
                        VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
                    imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                    break;
                case ERHIResourceState::DepthWrite:
                    accessFlags = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                    imageLayout =
                        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                    break;
                case ERHIResourceState::DepthRead:
                    accessFlags = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
                    imageLayout =
                        VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
                    break;
                case ERHIResourceState::ShaderResource:
                    accessFlags = VK_ACCESS_SHADER_READ_BIT;
                    imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    break;
                case ERHIResourceState::CopyDest:
                    accessFlags = VK_ACCESS_TRANSFER_WRITE_BIT;
                    imageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                    break;
                case ERHIResourceState::CopySource:
                    accessFlags = VK_ACCESS_TRANSFER_READ_BIT;
                    imageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                    break;
                case ERHIResourceState::Present:
                    accessFlags = 0;
                    imageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                    break;
                default:
                    accessFlags = 0;
                    imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    break;
            }
        }
    }  // namespace RHI

    // VulkanResource Implementation
    RHI::VulkanResource::VulkanResource(VulkanDevice* device,
                                        const std::string& debugName)
        : Device(device),
          DebugName(debugName),
          CurrentState(ERHIResourceState::Common) {}

    // VulkanBuffer Implementation
    RHI::VulkanBuffer::VulkanBuffer(VulkanDevice* device,
                                    const BufferDesc& desc)
        : VulkanResource(device, desc.DebugName),
          Desc(desc),
          Buffer(VK_NULL_HANDLE),
          Memory(VK_NULL_HANDLE),
          MappedData(nullptr) {
        if (!CreateBuffer()) {
            LOG_ERROR("Failed to create Vulkan buffer!");
            return;
        }

        if (!AllocateMemory()) {
            LOG_ERROR("Failed to allocate memory for Vulkan buffer!");
            return;
        }

        if (vkBindBufferMemory(Device->GetHandle(), Buffer, Memory, 0) !=
            VK_SUCCESS) {
            LOG_ERROR("Failed to bind buffer memory!");
            return;
        }
    }

    RHI::VulkanBuffer::~VulkanBuffer() {
        if (MappedData) {
            Unmap();
        }

        if (Buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(Device->GetHandle(), Buffer, nullptr);
        }

        if (Memory != VK_NULL_HANDLE) {
            vkFreeMemory(Device->GetHandle(), Memory, nullptr);
        }
    }

    bool RHI::VulkanBuffer::CreateBuffer() {
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = Desc.SizeInBytes;
        bufferInfo.usage =
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        // 根据缓冲区用途设置使用标志
        if (static_cast<uint32>(Desc.Flags) &
            static_cast<uint32>(ERHIResourceFlags::AllowUnorderedAccess)) {
            bufferInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        }
        if (Desc.Stride > 0) {
            bufferInfo.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        }
        if (static_cast<uint32>(Desc.Flags) &
            static_cast<uint32>(ERHIResourceFlags::AllowRenderTarget)) {
            bufferInfo.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        }

        return vkCreateBuffer(
                   Device->GetHandle(), &bufferInfo, nullptr, &Buffer) ==
               VK_SUCCESS;
    }

    bool RHI::VulkanBuffer::AllocateMemory() {
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(
            Device->GetHandle(), Buffer, &memRequirements);

        [[maybe_unused]] VkMemoryPropertyFlags properties = 0;
        if (static_cast<uint32>(Desc.Access) &
            static_cast<uint32>(ERHIAccessFlags::CPURead)) {
            properties |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        }
        if (static_cast<uint32>(Desc.Access) &
            static_cast<uint32>(ERHIAccessFlags::CPUWrite)) {
            properties |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        }
        if (!(static_cast<uint32>(Desc.Access) &
              static_cast<uint32>(ERHIAccessFlags::CPURead |
                                  ERHIAccessFlags::CPUWrite))) {
            properties |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        }

        // Memory = VulkanRHI::Get().GetMemoryAllocator()->AllocateMemory(
        //     memRequirements, properties);
        // return Memory != VK_NULL_HANDLE;
        return true;
    }

    void* RHI::VulkanBuffer::Map(uint32 subresource [[maybe_unused]]) {
        if (MappedData) {
            return MappedData;
        }

        if (vkMapMemory(Device->GetHandle(),
                        Memory,
                        0,
                        Desc.SizeInBytes,
                        0,
                        &MappedData) != VK_SUCCESS) {
            LOG_ERROR("Failed to map buffer memory!");
            return nullptr;
        }

        return MappedData;
    }

    void RHI::VulkanBuffer::Unmap(uint32 subresource [[maybe_unused]]) {
        if (MappedData) {
            vkUnmapMemory(Device->GetHandle(), Memory);
            MappedData = nullptr;
        }
    }

    // VulkanTexture Implementation
    RHI::VulkanTexture::VulkanTexture(VulkanDevice* device,
                                      const TextureDesc& desc)
        : VulkanResource(device, desc.DebugName),
          Desc(desc),
          Image(VK_NULL_HANDLE),
          Memory(VK_NULL_HANDLE),
          DefaultView(VK_NULL_HANDLE) {
        if (!CreateImage()) {
            LOG_ERROR("Failed to create Vulkan image!");
            return;
        }

        if (!AllocateMemory()) {
            LOG_ERROR("Failed to allocate memory for Vulkan image!");
            return;
        }

        if (vkBindImageMemory(Device->GetHandle(), Image, Memory, 0) !=
            VK_SUCCESS) {
            LOG_ERROR("Failed to bind image memory!");
            return;
        }

        if (!CreateDefaultView()) {
            LOG_ERROR("Failed to create default image view!");
            return;
        }
    }

    RHI::VulkanTexture::~VulkanTexture() {
        if (DefaultView != VK_NULL_HANDLE) {
            vkDestroyImageView(Device->GetHandle(), DefaultView, nullptr);
        }

        if (Image != VK_NULL_HANDLE) {
            vkDestroyImage(Device->GetHandle(), Image, nullptr);
        }

        if (Memory != VK_NULL_HANDLE) {
            vkFreeMemory(Device->GetHandle(), Memory, nullptr);
        }
    }

    bool RHI::VulkanTexture::CreateImage() {
        VkImageCreateInfo imageInfo = {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = GetVulkanFormat();
        imageInfo.extent.width = Desc.Width;
        imageInfo.extent.height = Desc.Height;
        imageInfo.extent.depth = Desc.Depth;
        imageInfo.mipLevels = Desc.MipLevels;
        imageInfo.arrayLayers = Desc.ArraySize;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage =
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        // 根据纹理用途设置使用标志
        if (static_cast<uint32>(Desc.Flags) &
            static_cast<uint32>(ERHIResourceFlags::AllowRenderTarget)) {
            imageInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }
        if (static_cast<uint32>(Desc.Flags) &
            static_cast<uint32>(ERHIResourceFlags::AllowDepthStencil)) {
            imageInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }
        if (!(static_cast<uint32>(Desc.Flags) &
              static_cast<uint32>(ERHIResourceFlags::DenyShaderResource))) {
            imageInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
        }
        if (static_cast<uint32>(Desc.Flags) &
            static_cast<uint32>(ERHIResourceFlags::AllowUnorderedAccess)) {
            imageInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
        }

        return vkCreateImage(
                   Device->GetHandle(), &imageInfo, nullptr, &Image) ==
               VK_SUCCESS;
    }

    bool RHI::VulkanTexture::AllocateMemory() {
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(
            Device->GetHandle(), Image, &memRequirements);

        // Memory = VulkanRHI::Get().GetMemoryAllocator()->AllocateMemory(
        //     memRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        // return Memory != VK_NULL_HANDLE;
        return true;
    }

    bool RHI::VulkanTexture::CreateDefaultView() {
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = Image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = GetVulkanFormat();
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = Desc.MipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = Desc.ArraySize;

        return vkCreateImageView(
                   Device->GetHandle(), &viewInfo, nullptr, &DefaultView) ==
               VK_SUCCESS;
    }

    VkFormat RHI::VulkanTexture::GetVulkanFormat() const {
        return ConvertToVulkanFormat(Desc.Format);
    }

    uint64 RHI::VulkanTexture::GetSize() const {
        // 简单的大小估算，实际应该基于格式和压缩等因素
        return uint64(Desc.Width) * Desc.Height * Desc.Depth * 4;
    }

    // VulkanShader Implementation
    RHI::VulkanShader::VulkanShader(VulkanDevice* device,
                                    const ShaderDesc& desc,
                                    const void* shaderData,
                                    size_t dataSize)
        : VulkanResource(device, desc.DebugName),
          Desc(desc),
          ShaderModule(VK_NULL_HANDLE),
          ShaderSize(dataSize) {
        if (!CreateShaderModule(shaderData, dataSize)) {
            LOG_ERROR("Failed to create shader module!");
        }
    }

    RHI::VulkanShader::~VulkanShader() {
        if (ShaderModule != VK_NULL_HANDLE) {
            vkDestroyShaderModule(Device->GetHandle(), ShaderModule, nullptr);
        }
    }

    bool RHI::VulkanShader::CreateShaderModule(const void* shaderData,
                                               size_t dataSize) {
        VkShaderModuleCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = dataSize;
        createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderData);

        return vkCreateShaderModule(
                   Device->GetHandle(), &createInfo, nullptr, &ShaderModule) ==
               VK_SUCCESS;
    }

    // VulkanRenderTargetView Implementation
    RHI::VulkanRenderTargetView::VulkanRenderTargetView(
        VulkanDevice* device,
        IRHIResource* resource,
        const RenderTargetViewDesc& desc)
        : VulkanResource(device, desc.DebugName),
          Desc(desc),
          Resource(resource),
          ImageView(VK_NULL_HANDLE) {
        if (!CreateImageView()) {
            LOG_ERROR("Failed to create render target view!");
        }
    }

    RHI::VulkanRenderTargetView::~VulkanRenderTargetView() {
        if (ImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(Device->GetHandle(), ImageView, nullptr);
        }
    }

    bool RHI::VulkanRenderTargetView::CreateImageView() {
        auto texture =
            dynamic_cast<VulkanTexture*>(dynamic_cast<IRHITexture*>(Resource));
        if (!texture) {
            return false;
        }

        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = texture->GetHandle();
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = ConvertToVulkanFormat(Desc.Format);
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = Desc.MipSlice;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = Desc.FirstArraySlice;
        viewInfo.subresourceRange.layerCount = Desc.ArraySize;

        return vkCreateImageView(
                   Device->GetHandle(), &viewInfo, nullptr, &ImageView) ==
               VK_SUCCESS;
    }

    // VulkanDepthStencilView Implementation
    RHI::VulkanDepthStencilView::VulkanDepthStencilView(
        VulkanDevice* device,
        IRHIResource* resource,
        const DepthStencilViewDesc& desc)
        : VulkanResource(device, desc.DebugName),
          Desc(desc),
          Resource(resource),
          ImageView(VK_NULL_HANDLE) {
        if (!CreateImageView()) {
            LOG_ERROR("Failed to create depth stencil view!");
        }
    }

    RHI::VulkanDepthStencilView::~VulkanDepthStencilView() {
        if (ImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(Device->GetHandle(), ImageView, nullptr);
        }
    }

    bool RHI::VulkanDepthStencilView::CreateImageView() {
        if (Resource->GetResourceDimension() ==
            ERHIResourceDimension::Texture2D) {
            auto texture = dynamic_cast<VulkanTexture*>(
                dynamic_cast<IRHITexture*>(Resource));
            if (!texture) {
                return false;
            }

            VkImageViewCreateInfo viewInfo = {};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = texture->GetHandle();
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = ConvertToVulkanFormat(Desc.Format);
            viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            if (Desc.Format == EPixelFormat::D24_UNORM_S8_UINT ||
                Desc.Format == EPixelFormat::D32_FLOAT_S8X24_UINT) {
                viewInfo.subresourceRange.aspectMask |=
                    VK_IMAGE_ASPECT_STENCIL_BIT;
            }
            viewInfo.subresourceRange.baseMipLevel = Desc.MipSlice;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = Desc.FirstArraySlice;
            viewInfo.subresourceRange.layerCount = Desc.ArraySize;

            return vkCreateImageView(
                       Device->GetHandle(), &viewInfo, nullptr, &ImageView) ==
                   VK_SUCCESS;
        }

        return false;
    }

    // VulkanShaderResourceView Implementation
    RHI::VulkanShaderResourceView::VulkanShaderResourceView(
        VulkanDevice* device,
        IRHIResource* resource,
        const ShaderResourceViewDesc& desc)
        : VulkanResource(device, desc.DebugName),
          Desc(desc),
          Resource(resource),
          ImageView(VK_NULL_HANDLE),
          BufferView(VK_NULL_HANDLE) {
        if (!CreateView()) {
            LOG_ERROR("Failed to create shader resource view!");
        }
    }

    RHI::VulkanShaderResourceView::~VulkanShaderResourceView() {
        if (ImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(Device->GetHandle(), ImageView, nullptr);
        }
        if (BufferView != VK_NULL_HANDLE) {
            vkDestroyBufferView(Device->GetHandle(), BufferView, nullptr);
        }
    }

    bool RHI::VulkanShaderResourceView::CreateView() {
        if (Resource->GetResourceDimension() == ERHIResourceDimension::Buffer) {
            auto buffer = dynamic_cast<VulkanBuffer*>(
                dynamic_cast<IRHIBuffer*>(Resource));
            if (!buffer) {
                return false;
            }

            VkBufferViewCreateInfo viewInfo = {};
            viewInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
            viewInfo.buffer = buffer->GetHandle();
            viewInfo.format = ConvertToVulkanFormat(Desc.Format);
            viewInfo.offset = 0;
            viewInfo.range = VK_WHOLE_SIZE;

            return vkCreateBufferView(
                       Device->GetHandle(), &viewInfo, nullptr, &BufferView) ==
                   VK_SUCCESS;
        } else {
            auto texture = dynamic_cast<VulkanTexture*>(
                dynamic_cast<IRHITexture*>(Resource));
            if (!texture) {
                return false;
            }

            VkImageViewCreateInfo viewInfo = {};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = texture->GetHandle();
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = ConvertToVulkanFormat(Desc.Format);
            viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = Desc.MostDetailedMip;
            viewInfo.subresourceRange.levelCount = Desc.MipLevels;
            viewInfo.subresourceRange.baseArrayLayer = Desc.FirstArraySlice;
            viewInfo.subresourceRange.layerCount = Desc.ArraySize;

            return vkCreateImageView(
                       Device->GetHandle(), &viewInfo, nullptr, &ImageView) ==
                   VK_SUCCESS;
        }
    }

    // VulkanUnorderedAccessView Implementation
    RHI::VulkanUnorderedAccessView::VulkanUnorderedAccessView(
        VulkanDevice* device,
        IRHIResource* resource,
        const UnorderedAccessViewDesc& desc)
        : VulkanResource(device, desc.DebugName),
          Desc(desc),
          Resource(resource),
          ImageView(VK_NULL_HANDLE),
          BufferView(VK_NULL_HANDLE) {
        if (!CreateView()) {
            LOG_ERROR("Failed to create unordered access view!");
        }
    }

    RHI::VulkanUnorderedAccessView::~VulkanUnorderedAccessView() {
        if (ImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(Device->GetHandle(), ImageView, nullptr);
        }
        if (BufferView != VK_NULL_HANDLE) {
            vkDestroyBufferView(Device->GetHandle(), BufferView, nullptr);
        }
    }

    bool RHI::VulkanUnorderedAccessView::CreateView() {
        if (Resource->GetResourceDimension() == ERHIResourceDimension::Buffer) {
            auto buffer = dynamic_cast<VulkanBuffer*>(
                dynamic_cast<IRHIBuffer*>(Resource));
            if (!buffer) {
                return false;
            }

            VkBufferViewCreateInfo viewInfo = {};
            viewInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
            viewInfo.buffer = buffer->GetHandle();
            viewInfo.format = ConvertToVulkanFormat(Desc.Format);
            viewInfo.offset = 0;
            viewInfo.range = VK_WHOLE_SIZE;

            return vkCreateBufferView(
                       Device->GetHandle(), &viewInfo, nullptr, &BufferView) ==
                   VK_SUCCESS;
        } else {
            auto texture = dynamic_cast<VulkanTexture*>(
                dynamic_cast<IRHITexture*>(Resource));
            if (!texture) {
                return false;
            }

            VkImageViewCreateInfo viewInfo = {};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = texture->GetHandle();
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = ConvertToVulkanFormat(Desc.Format);
            viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = Desc.MipSlice;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = Desc.FirstArraySlice;
            viewInfo.subresourceRange.layerCount = Desc.ArraySize;

            return vkCreateImageView(
                       Device->GetHandle(), &viewInfo, nullptr, &ImageView) ==
                   VK_SUCCESS;
        }
    }

}  // namespace Engine
