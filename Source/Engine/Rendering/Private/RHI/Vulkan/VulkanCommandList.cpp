
#include "VulkanCommandList.h"

#include "Core/Public/Log/LogSystem.h"

namespace Engine {
    namespace RHI {

        namespace {
            VkPrimitiveTopology ConvertToVulkanPrimitiveTopology(
                EPrimitiveType type) {
                switch (type) {
                    case EPrimitiveType::Points:
                        return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
                    case EPrimitiveType::Lines:
                        return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
                    case EPrimitiveType::LineStrip:
                        return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
                    case EPrimitiveType::Triangles:
                        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                    case EPrimitiveType::TriangleStrip:
                        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
                    default:
                        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                }
            }

            VkCommandPool CreateCommandPool(VulkanDevice* device,
                                            ECommandListType type) {
                VkCommandPoolCreateInfo poolInfo = {};
                poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
                poolInfo.flags =
                    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

                switch (type) {
                    case ECommandListType::Direct:
                        poolInfo.queueFamilyIndex =
                            device->GetGraphicsQueueFamilyIndex();
                        break;
                    case ECommandListType::Compute:
                        poolInfo.queueFamilyIndex =
                            device->GetComputeQueueFamilyIndex();
                        break;
                    case ECommandListType::Copy:
                        poolInfo.queueFamilyIndex =
                            device->GetTransferQueueFamilyIndex();
                        break;
                    default:
                        LOG_ERROR("Invalid command list type!");
                        return VK_NULL_HANDLE;
                }

                VkCommandPool commandPool;
                if (vkCreateCommandPool(device->GetHandle(),
                                        &poolInfo,
                                        nullptr,
                                        &commandPool) != VK_SUCCESS) {
                    LOG_ERROR("Failed to create command pool!");
                    return VK_NULL_HANDLE;
                }

                return commandPool;
            }
        }  // namespace

        // VulkanCommandBuffer Implementation
        VulkanCommandBuffer::VulkanCommandBuffer(VulkanDevice* device,
                                                 VkCommandPool commandPool,
                                                 VkCommandBufferLevel level)
            : Device(device),
              CommandPool(commandPool),
              CommandBuffer(VK_NULL_HANDLE) {
            VkCommandBufferAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = commandPool;
            allocInfo.level = level;
            allocInfo.commandBufferCount = 1;

            if (vkAllocateCommandBuffers(Device->GetHandle(),
                                         &allocInfo,
                                         &CommandBuffer) != VK_SUCCESS) {
                LOG_ERROR("Failed to allocate command buffer!");
            }
        }

        VulkanCommandBuffer::~VulkanCommandBuffer() {
            if (CommandBuffer != VK_NULL_HANDLE) {
                vkFreeCommandBuffers(
                    Device->GetHandle(), CommandPool, 1, &CommandBuffer);
                CommandBuffer = VK_NULL_HANDLE;
            }
        }

        bool VulkanCommandBuffer::Begin(VkCommandBufferUsageFlags flags) {
            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = flags;

            return vkBeginCommandBuffer(CommandBuffer, &beginInfo) ==
                   VK_SUCCESS;
        }

        bool VulkanCommandBuffer::End() {
            return vkEndCommandBuffer(CommandBuffer) == VK_SUCCESS;
        }

        void VulkanCommandBuffer::Reset() {
            vkResetCommandBuffer(CommandBuffer, 0);
        }

        // VulkanCommandList Implementation
        VulkanCommandList::VulkanCommandList(VulkanDevice* device,
                                             ECommandListType type)
            : Device(device),
              Type(type),
              CurrentCommandBuffer(nullptr),
              IsInRecordingState(false),
              IsInRenderPass(false) {
            State.CurrentRenderPass = VK_NULL_HANDLE;
            State.CurrentFramebuffer = VK_NULL_HANDLE;
            State.CurrentPipeline = VK_NULL_HANDLE;
            State.CurrentPipelineLayout = VK_NULL_HANDLE;
        }

        VulkanCommandList::~VulkanCommandList() { delete CurrentCommandBuffer; }

        void VulkanCommandList::Reset(IRHICommandAllocator* allocator) {
            auto vulkanAllocator =
                dynamic_cast<VulkanCommandAllocator*>(allocator);
            if (!vulkanAllocator) {
                LOG_ERROR("Invalid command allocator!");
                return;
            }

            if (CurrentCommandBuffer) {
                CurrentCommandBuffer->Reset();
            } else {
                CurrentCommandBuffer = new VulkanCommandBuffer(
                    Device, vulkanAllocator->GetCommandPool());
            }

            IsInRecordingState = false;
            IsInRenderPass = false;
        }

        void VulkanCommandList::Close() {
            if (IsInRenderPass) {
                EndRenderPass();
            }

            if (IsInRecordingState && CurrentCommandBuffer) {
                CurrentCommandBuffer->End();
                IsInRecordingState = false;
            }
        }

        void VulkanCommandList::BeginEvent(const char* name) {
            if (!IsInRecordingState) {
                return;
            }

            VkDebugUtilsLabelEXT label = {};
            label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
            label.pLabelName = name;
            label.color[0] = 1.0f;
            label.color[1] = 1.0f;
            label.color[2] = 1.0f;
            label.color[3] = 1.0f;

            auto func = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetDeviceProcAddr(
                Device->GetHandle(), "vkCmdBeginDebugUtilsLabelEXT");
            if (func) {
                func(GetCommandBuffer(), &label);
            }
        }

        void VulkanCommandList::EndEvent() {
            if (!IsInRecordingState) {
                return;
            }

            auto func = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetDeviceProcAddr(
                Device->GetHandle(), "vkCmdEndDebugUtilsLabelEXT");
            if (func) {
                func(GetCommandBuffer());
            }
        }

        void VulkanCommandList::ResourceBarrier(IRHIResource* resource,
                                                ERHIResourceState before,
                                                ERHIResourceState after) {
            if (!IsInRecordingState || !resource) {
                return;
            }

            // 处理图像布局转换
            if (resource->GetResourceDimension() !=
                ERHIResourceDimension::Buffer) {
                auto texture = dynamic_cast<VulkanTexture*>(resource);
                if (texture) {
                    VkImageLayout oldLayout, newLayout;
                    VkAccessFlags srcAccessMask, dstAccessMask;
                    ConvertResourceState(before, srcAccessMask, oldLayout);
                    ConvertResourceState(after, dstAccessMask, newLayout);

                    VkImageMemoryBarrier barrier = {};
                    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    barrier.oldLayout = oldLayout;
                    barrier.newLayout = newLayout;
                    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.image = texture->GetHandle();
                    barrier.subresourceRange.aspectMask =
                        VK_IMAGE_ASPECT_COLOR_BIT;
                    barrier.subresourceRange.baseMipLevel = 0;
                    barrier.subresourceRange.levelCount =
                        VK_REMAINING_MIP_LEVELS;
                    barrier.subresourceRange.baseArrayLayer = 0;
                    barrier.subresourceRange.layerCount =
                        VK_REMAINING_ARRAY_LAYERS;
                    barrier.srcAccessMask = srcAccessMask;
                    barrier.dstAccessMask = dstAccessMask;

                    vkCmdPipelineBarrier(GetCommandBuffer(),
                                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                         0,
                                         0,
                                         nullptr,
                                         0,
                                         nullptr,
                                         1,
                                         &barrier);
                }
            }
            // 处理缓冲区内存屏障
            else {
                auto buffer = dynamic_cast<VulkanBuffer*>(resource);
                if (buffer) {
                    VkAccessFlags srcAccessMask, dstAccessMask;
                    VkImageLayout dummy;
                    ConvertResourceState(before, srcAccessMask, dummy);
                    ConvertResourceState(after, dstAccessMask, dummy);

                    VkBufferMemoryBarrier barrier = {};
                    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                    barrier.srcAccessMask = srcAccessMask;
                    barrier.dstAccessMask = dstAccessMask;
                    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.buffer = buffer->GetHandle();
                    barrier.offset = 0;
                    barrier.size = VK_WHOLE_SIZE;

                    vkCmdPipelineBarrier(GetCommandBuffer(),
                                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                         0,
                                         0,
                                         nullptr,
                                         1,
                                         &barrier,
                                         0,
                                         nullptr);
                }
            }
        }

        void VulkanCommandList::SetRenderTargets(
            uint32 numRenderTargets,
            IRHIRenderTargetView* const* renderTargets,
            IRHIDepthStencilView* depthStencil) {
            if (IsInRenderPass) {
                EndRenderPass();
            }

            State.RenderTargetViews.clear();
            State.DepthStencilView = VK_NULL_HANDLE;

            for (uint32 i = 0; i < numRenderTargets; ++i) {
                if (renderTargets[i]) {
                    auto rtv =
                        static_cast<VulkanRenderTargetView*>(renderTargets[i]);
                    State.RenderTargetViews.push_back(rtv->GetHandle());
                }
            }

            if (depthStencil) {
                auto dsv = static_cast<VulkanDepthStencilView*>(depthStencil);
                State.DepthStencilView = dsv->GetHandle();
            }

            BeginRenderPass();
        }

        void VulkanCommandList::ClearRenderTargetView(
            IRHIRenderTargetView* renderTarget, const float* clearColor) {
            if (!IsInRecordingState || !renderTarget) {
                return;
            }

            auto rtv = static_cast<VulkanRenderTargetView*>(renderTarget);
            VkClearColorValue clearValue = {};
            memcpy(clearValue.float32, clearColor, sizeof(float) * 4);

            VkImageSubresourceRange range = {};
            range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            range.baseMipLevel = 0;
            range.levelCount = 1;
            range.baseArrayLayer = 0;
            range.layerCount = 1;

            vkCmdClearColorImage(
                GetCommandBuffer(),
                dynamic_cast<VulkanTexture*>(renderTarget->GetResource())
                    ->GetHandle(),
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                &clearValue,
                1,
                &range);
        }

        void VulkanCommandList::ClearDepthStencilView(
            IRHIDepthStencilView* depthStencil, float depth, uint8 stencil) {
            if (!IsInRecordingState || !depthStencil) {
                return;
            }

            auto dsv = static_cast<VulkanDepthStencilView*>(depthStencil);
            VkClearDepthStencilValue clearValue = {};
            clearValue.depth = depth;
            clearValue.stencil = stencil;

            VkImageSubresourceRange range = {};
            range.aspectMask =
                VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            range.baseMipLevel = 0;
            range.levelCount = 1;
            range.baseArrayLayer = 0;
            range.layerCount = 1;

            vkCmdClearDepthStencilImage(
                GetCommandBuffer(),
                dynamic_cast<VulkanTexture*>(depthStencil->GetResource())
                    ->GetHandle(),
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                &clearValue,
                1,
                &range);
        }

        void VulkanCommandList::SetViewport(const Viewport& viewport) {
            if (!IsInRecordingState) {
                return;
            }

            VkViewport vkViewport = {};
            vkViewport.x = viewport.TopLeftX;
            vkViewport.y = viewport.TopLeftY;
            vkViewport.width = viewport.Width;
            vkViewport.height = viewport.Height;
            vkViewport.minDepth = viewport.MinDepth;
            vkViewport.maxDepth = viewport.MaxDepth;

            vkCmdSetViewport(GetCommandBuffer(), 0, 1, &vkViewport);
        }

        void VulkanCommandList::SetScissorRect(const ScissorRect& scissor) {
            if (!IsInRecordingState) {
                return;
            }

            VkRect2D vkScissor = {};
            vkScissor.offset = {scissor.Left, scissor.Top};
            vkScissor.extent = {
                static_cast<uint32_t>(scissor.Right - scissor.Left),
                static_cast<uint32_t>(scissor.Bottom - scissor.Top)};

            vkCmdSetScissor(GetCommandBuffer(), 0, 1, &vkScissor);
        }

        void VulkanCommandList::SetPrimitiveTopology(
            EPrimitiveType primitiveType) {
            // 在Vulkan中，图元拓扑是管线状态的一部分，需要在创建管线时设置
            // 这里可以缓存设置，在创建管线时使用
        }

        void VulkanCommandList::SetVertexBuffers(
            uint32 startSlot,
            uint32 numBuffers,
            const VertexBufferView* views) {
            if (!IsInRecordingState) {
                return;
            }

            std::vector<VkBuffer> buffers(numBuffers);
            std::vector<VkDeviceSize> offsets(numBuffers);

            for (uint32 i = 0; i < numBuffers; ++i) {
                auto buffer = dynamic_cast<VulkanBuffer*>(views[i].Buffer);
                buffers[i] = buffer->GetHandle();
                offsets[i] = views[i].Offset;
            }

            vkCmdBindVertexBuffers(GetCommandBuffer(),
                                   startSlot,
                                   numBuffers,
                                   buffers.data(),
                                   offsets.data());
        }

        void VulkanCommandList::SetIndexBuffer(const IndexBufferView& view) {
            if (!IsInRecordingState) {
                return;
            }

            auto buffer = dynamic_cast<VulkanBuffer*>(view.Buffer);
            VkIndexType indexType = (view.Format == EPixelFormat::R16_UINT)
                                        ? VK_INDEX_TYPE_UINT16
                                        : VK_INDEX_TYPE_UINT32;

            vkCmdBindIndexBuffer(GetCommandBuffer(),
                                 buffer->GetHandle(),
                                 view.Offset,
                                 indexType);
        }

        void VulkanCommandList::Draw(uint32 vertexCount,
                                     uint32 startVertexLocation) {
            if (!IsInRecordingState) {
                return;
            }

            vkCmdDraw(
                GetCommandBuffer(), vertexCount, 1, startVertexLocation, 0);
        }

        void VulkanCommandList::DrawIndexed(uint32 indexCount,
                                            uint32 startIndexLocation,
                                            int32 baseVertexLocation) {
            if (!IsInRecordingState) {
                return;
            }

            vkCmdDrawIndexed(GetCommandBuffer(),
                             indexCount,
                             1,
                             startIndexLocation,
                             baseVertexLocation,
                             0);
        }

        void VulkanCommandList::DrawInstanced(uint32 vertexCountPerInstance,
                                              uint32 instanceCount,
                                              uint32 startVertexLocation,
                                              uint32 startInstanceLocation) {
            if (!IsInRecordingState) {
                return;
            }

            vkCmdDraw(GetCommandBuffer(),
                      vertexCountPerInstance,
                      instanceCount,
                      startVertexLocation,
                      startInstanceLocation);
        }

        void VulkanCommandList::DrawIndexedInstanced(
            uint32 indexCountPerInstance,
            uint32 instanceCount,
            uint32 startIndexLocation,
            int32 baseVertexLocation,
            uint32 startInstanceLocation) {
            if (!IsInRecordingState) {
                return;
            }

            vkCmdDrawIndexed(GetCommandBuffer(),
                             indexCountPerInstance,
                             instanceCount,
                             startIndexLocation,
                             baseVertexLocation,
                             startInstanceLocation);
        }

        void VulkanCommandList::Dispatch(uint32 threadGroupCountX,
                                         uint32 threadGroupCountY,
                                         uint32 threadGroupCountZ) {
            if (!IsInRecordingState) {
                return;
            }

            vkCmdDispatch(GetCommandBuffer(),
                          threadGroupCountX,
                          threadGroupCountY,
                          threadGroupCountZ);
        }

        void VulkanCommandList::CopyBuffer(IRHIBuffer* dest,
                                           uint64 destOffset,
                                           IRHIBuffer* source,
                                           uint64 sourceOffset,
                                           uint64 size) {
            if (!IsInRecordingState || !dest || !source) {
                return;
            }

            auto srcBuffer = static_cast<VulkanBuffer*>(source);
            auto dstBuffer = static_cast<VulkanBuffer*>(dest);

            VkBufferCopy copyRegion = {};
            copyRegion.srcOffset = sourceOffset;
            copyRegion.dstOffset = destOffset;
            copyRegion.size = size;

            vkCmdCopyBuffer(GetCommandBuffer(),
                            srcBuffer->GetHandle(),
                            dstBuffer->GetHandle(),
                            1,
                            &copyRegion);
        }

        void VulkanCommandList::CopyTexture(IRHITexture* dest,
                                            uint32 destSubresource,
                                            uint32 destX,
                                            uint32 destY,
                                            uint32 destZ,
                                            IRHITexture* source,
                                            uint32 sourceSubresource) {
            if (!IsInRecordingState || !dest || !source) {
                return;
            }

            auto srcTexture = static_cast<VulkanTexture*>(source);
            auto dstTexture = static_cast<VulkanTexture*>(dest);

            VkImageCopy copyRegion = {};
            copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegion.srcSubresource.mipLevel = 0;
            copyRegion.srcSubresource.baseArrayLayer = 0;
            copyRegion.srcSubresource.layerCount = 1;
            copyRegion.srcOffset = {0, 0, 0};
            copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegion.dstSubresource.mipLevel = 0;
            copyRegion.dstSubresource.baseArrayLayer = 0;
            copyRegion.dstSubresource.layerCount = 1;
            copyRegion.dstOffset = {static_cast<int32_t>(destX),
                                    static_cast<int32_t>(destY),
                                    static_cast<int32_t>(destZ)};
            copyRegion.extent = {dstTexture->GetWidth(),
                                 dstTexture->GetHeight(),
                                 dstTexture->GetDepth()};

            vkCmdCopyImage(GetCommandBuffer(),
                           srcTexture->GetHandle(),
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           dstTexture->GetHandle(),
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1,
                           &copyRegion);
        }

        VkCommandBuffer VulkanCommandList::GetCommandBuffer() const {
            return CurrentCommandBuffer ? CurrentCommandBuffer->GetHandle()
                                        : VK_NULL_HANDLE;
        }

        void VulkanCommandList::BeginRenderPass() {
            if (!IsInRecordingState || IsInRenderPass) {
                return;
            }

            // TODO: 创建RenderPass和Framebuffer
            IsInRenderPass = true;
        }

        void VulkanCommandList::EndRenderPass() {
            if (!IsInRecordingState || !IsInRenderPass) {
                return;
            }

            vkCmdEndRenderPass(GetCommandBuffer());
            IsInRenderPass = false;
        }

        void VulkanCommandList::EnsureCommandBuffer() {
            if (!CurrentCommandBuffer) {
                LOG_ERROR("No command buffer available!");
                return;
            }

            if (!IsInRecordingState) {
                CurrentCommandBuffer->Begin();
                IsInRecordingState = true;
            }
        }

        // VulkanCommandAllocator Implementation
        VulkanCommandAllocator::VulkanCommandAllocator(VulkanDevice* device,
                                                       ECommandListType type)
            : Device(device), Type(type), CommandPool(VK_NULL_HANDLE) {
            CommandPool = CreateCommandPool(device, type);
        }

        VulkanCommandAllocator::~VulkanCommandAllocator() {
            if (CommandPool != VK_NULL_HANDLE) {
                vkDestroyCommandPool(Device->GetHandle(), CommandPool, nullptr);
                CommandPool = VK_NULL_HANDLE;
            }
        }

        void VulkanCommandAllocator::Reset() {
            if (CommandPool != VK_NULL_HANDLE) {
                vkResetCommandPool(Device->GetHandle(), CommandPool, 0);
            }
        }

    }  // namespace RHI
}  // namespace Engine
