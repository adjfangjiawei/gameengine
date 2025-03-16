
#include "VulkanCommandList.h"

#include "Core/Public/Log/LogSystem.h"

namespace Engine {
    namespace RHI {

        namespace {
            [[maybe_unused]] VkPrimitiveTopology
            ConvertToVulkanPrimitiveTopology(EPrimitiveType type) {
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

        VulkanCommandList::~VulkanCommandList() {
            // 清理命令缓冲区
            delete CurrentCommandBuffer;

            // 清理描述符池
            if (State.DescriptorPool != VK_NULL_HANDLE) {
                vkDestroyDescriptorPool(
                    Device->GetHandle(), State.DescriptorPool, nullptr);
            }
            if (State.SRVDescriptorPool != VK_NULL_HANDLE) {
                vkDestroyDescriptorPool(
                    Device->GetHandle(), State.SRVDescriptorPool, nullptr);
            }
            if (State.UAVDescriptorPool != VK_NULL_HANDLE) {
                vkDestroyDescriptorPool(
                    Device->GetHandle(), State.UAVDescriptorPool, nullptr);
            }

            // 清理描述符集布局
            for (auto layout : State.DescriptorSetLayouts) {
                vkDestroyDescriptorSetLayout(
                    Device->GetHandle(), layout, nullptr);
            }
            for (auto layout : State.SRVDescriptorSetLayouts) {
                vkDestroyDescriptorSetLayout(
                    Device->GetHandle(), layout, nullptr);
            }
            for (auto layout : State.UAVDescriptorSetLayouts) {
                vkDestroyDescriptorSetLayout(
                    Device->GetHandle(), layout, nullptr);
            }

            // 清理管线和管线布局
            if (State.CurrentPipeline != VK_NULL_HANDLE) {
                vkDestroyPipeline(
                    Device->GetHandle(), State.CurrentPipeline, nullptr);
            }
            if (State.CurrentPipelineLayout != VK_NULL_HANDLE) {
                vkDestroyPipelineLayout(
                    Device->GetHandle(), State.CurrentPipelineLayout, nullptr);
            }

            // 清理渲染通道和帧缓冲
            if (State.CurrentRenderPass != VK_NULL_HANDLE) {
                vkDestroyRenderPass(
                    Device->GetHandle(), State.CurrentRenderPass, nullptr);
            }
            if (State.CurrentFramebuffer != VK_NULL_HANDLE) {
                vkDestroyFramebuffer(
                    Device->GetHandle(), State.CurrentFramebuffer, nullptr);
            }

            // 注意：不需要显式清理着色器模块，因为它们由VulkanShader对象管理
        }

        void VulkanCommandList::Reset() {
            if (CurrentCommandBuffer) {
                CurrentCommandBuffer->Reset();
                IsInRecordingState = false;
                IsInRenderPass = false;
            }
        }

        void VulkanCommandList::ResetWithAllocator(
            IRHICommandAllocator* allocator) {
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
                        dynamic_cast<VulkanRenderTargetView*>(renderTargets[i]);
                    State.RenderTargetViews.push_back(rtv->GetHandle());
                }
            }

            if (depthStencil) {
                auto dsv = dynamic_cast<VulkanDepthStencilView*>(depthStencil);
                State.DepthStencilView = dsv->GetHandle();
            }

            BeginRenderPass();
        }

        void VulkanCommandList::ClearRenderTargetView(
            IRHIRenderTargetView* renderTarget, const float* clearColor) {
            if (!IsInRecordingState || !renderTarget) {
                return;
            }

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
            [[maybe_unused]] EPrimitiveType primitiveType) {
            // Store the primitive type for later use when creating the pipeline
            // State.PrimitiveTopology =
            //     ConvertToVulkanPrimitiveTopology(primitiveType);
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

            auto srcBuffer = dynamic_cast<VulkanBuffer*>(source);
            auto dstBuffer = dynamic_cast<VulkanBuffer*>(dest);

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

            auto srcTexture = dynamic_cast<VulkanTexture*>(source);
            auto dstTexture = dynamic_cast<VulkanTexture*>(dest);

            VkImageCopy copyRegion = {};
            copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegion.srcSubresource.mipLevel = sourceSubresource;
            copyRegion.srcSubresource.baseArrayLayer = 0;
            copyRegion.srcSubresource.layerCount = 1;
            copyRegion.srcOffset = {0, 0, 0};
            copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegion.dstSubresource.mipLevel = destSubresource;
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

            // 创建颜色附件描述
            std::vector<VkAttachmentDescription> attachments;
            std::vector<VkAttachmentReference> colorAttachmentRefs;

            // 处理颜色附件
            for (size_t i = 0; i < State.RenderTargetViews.size(); ++i) {
                VkAttachmentDescription colorAttachment = {};
                colorAttachment.format =
                    VK_FORMAT_R8G8B8A8_UNORM;  // 需要从RTV中获取实际格式
                colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
                colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                colorAttachment.stencilStoreOp =
                    VK_ATTACHMENT_STORE_OP_DONT_CARE;
                colorAttachment.initialLayout =
                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                colorAttachment.finalLayout =
                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                attachments.push_back(colorAttachment);

                VkAttachmentReference colorAttachmentRef = {};
                colorAttachmentRef.attachment = static_cast<uint32_t>(i);
                colorAttachmentRef.layout =
                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                colorAttachmentRefs.push_back(colorAttachmentRef);
            }

            // 处理深度模板附件
            VkAttachmentReference depthAttachmentRef = {};
            if (State.DepthStencilView != VK_NULL_HANDLE) {
                VkAttachmentDescription depthAttachment = {};
                depthAttachment.format =
                    VK_FORMAT_D24_UNORM_S8_UINT;  // 需要从DSV中获取实际格式
                depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
                depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
                depthAttachment.initialLayout =
                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                depthAttachment.finalLayout =
                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                attachments.push_back(depthAttachment);

                depthAttachmentRef.attachment =
                    static_cast<uint32_t>(attachments.size() - 1);
                depthAttachmentRef.layout =
                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            }

            // 创建子通道描述
            VkSubpassDescription subpass = {};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount =
                static_cast<uint32_t>(colorAttachmentRefs.size());
            subpass.pColorAttachments = colorAttachmentRefs.data();
            if (State.DepthStencilView != VK_NULL_HANDLE) {
                subpass.pDepthStencilAttachment = &depthAttachmentRef;
            }

            // 创建子通道依赖
            VkSubpassDependency dependency = {};
            dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
            dependency.dstSubpass = 0;
            dependency.srcStageMask =
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.srcAccessMask = 0;
            dependency.dstStageMask =
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                       VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

            // 创建渲染通道
            VkRenderPassCreateInfo renderPassInfo = {};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassInfo.attachmentCount =
                static_cast<uint32_t>(attachments.size());
            renderPassInfo.pAttachments = attachments.data();
            renderPassInfo.subpassCount = 1;
            renderPassInfo.pSubpasses = &subpass;
            renderPassInfo.dependencyCount = 1;
            renderPassInfo.pDependencies = &dependency;

            VkRenderPass renderPass;
            if (vkCreateRenderPass(Device->GetHandle(),
                                   &renderPassInfo,
                                   nullptr,
                                   &renderPass) != VK_SUCCESS) {
                LOG_ERROR("Failed to create render pass!");
                return;
            }

            // 创建帧缓冲
            std::vector<VkImageView> attachmentViews;
            attachmentViews.insert(attachmentViews.end(),
                                   State.RenderTargetViews.begin(),
                                   State.RenderTargetViews.end());
            if (State.DepthStencilView != VK_NULL_HANDLE) {
                attachmentViews.push_back(State.DepthStencilView);
            }

            VkFramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount =
                static_cast<uint32_t>(attachmentViews.size());
            framebufferInfo.pAttachments = attachmentViews.data();
            framebufferInfo.width = 1920;   // 需要从RTV中获取实际宽度
            framebufferInfo.height = 1080;  // 需要从RTV中获取实际高度
            framebufferInfo.layers = 1;

            VkFramebuffer framebuffer;
            if (vkCreateFramebuffer(Device->GetHandle(),
                                    &framebufferInfo,
                                    nullptr,
                                    &framebuffer) != VK_SUCCESS) {
                vkDestroyRenderPass(Device->GetHandle(), renderPass, nullptr);
                LOG_ERROR("Failed to create framebuffer!");
                return;
            }

            // 开始渲染通道
            VkRenderPassBeginInfo renderPassBegin = {};
            renderPassBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassBegin.renderPass = renderPass;
            renderPassBegin.framebuffer = framebuffer;
            renderPassBegin.renderArea.offset = {0, 0};
            renderPassBegin.renderArea.extent = {
                1920, 1080};  // 需要从RTV中获取实际尺寸

            std::vector<VkClearValue> clearValues(attachmentViews.size());
            vkCmdBeginRenderPass(GetCommandBuffer(),
                                 &renderPassBegin,
                                 VK_SUBPASS_CONTENTS_INLINE);

            // 保存状态
            State.CurrentRenderPass = renderPass;
            State.CurrentFramebuffer = framebuffer;
            IsInRenderPass = true;
        }

        void VulkanCommandList::SetBlendState(
            const VkPipelineColorBlendStateCreateInfo& blendState) {
            if (!IsInRecordingState) {
                return;
            }
            // 存储混合状态以在创建管线时使用
            State.BlendState = blendState;
        }

        void VulkanCommandList::SetRasterizerState(
            const VkPipelineRasterizationStateCreateInfo& rasterizerState,
            const VkPipelineMultisampleStateCreateInfo& multisampleState) {
            if (!IsInRecordingState) {
                return;
            }
            // 存储光栅化和多重采样状态以在创建管线时使用
            State.RasterizerState = rasterizerState;
            State.MultisampleState = multisampleState;
        }

        void VulkanCommandList::SetDepthStencilState(
            const VkPipelineDepthStencilStateCreateInfo& depthStencilState) {
            if (!IsInRecordingState) {
                return;
            }
            // 存储深度模板状态以在创建管线时使用
            State.DepthStencilState = depthStencilState;
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
        void VulkanCommandList::SetShader(EShaderType type,
                                          IRHIShader* shader) {
            if (!IsInRecordingState || !shader) {
                return;
            }

            auto vulkanShader = dynamic_cast<VulkanShader*>(shader);
            if (!vulkanShader) {
                LOG_ERROR("Invalid shader type!");
                return;
            }

            // 存储着色器模块以在创建管线时使用
            switch (type) {
                case EShaderType::Vertex:
                    State.VertexShader = vulkanShader->GetHandle();
                    break;
                case EShaderType::Pixel:
                    State.PixelShader = vulkanShader->GetHandle();
                    break;
                case EShaderType::Geometry:
                    State.GeometryShader = vulkanShader->GetHandle();
                    break;
                case EShaderType::Compute:
                    State.ComputeShader = vulkanShader->GetHandle();
                    break;
                case EShaderType::Hull:
                    State.HullShader = vulkanShader->GetHandle();
                    break;
                case EShaderType::Domain:
                    State.DomainShader = vulkanShader->GetHandle();
                    break;
                default:
                    LOG_ERROR("Unsupported shader type!");
                    break;
            }

            // 如果是计算着色器，需要更新管线布局和描述符集
            if (type == EShaderType::Compute &&
                State.ComputeShader != VK_NULL_HANDLE) {
                VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
                pipelineLayoutInfo.sType =
                    VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
                // TODO: 设置描述符集布局和推送常量范围

                if (State.CurrentPipelineLayout != VK_NULL_HANDLE) {
                    vkDestroyPipelineLayout(Device->GetHandle(),
                                            State.CurrentPipelineLayout,
                                            nullptr);
                }

                if (vkCreatePipelineLayout(Device->GetHandle(),
                                           &pipelineLayoutInfo,
                                           nullptr,
                                           &State.CurrentPipelineLayout) !=
                    VK_SUCCESS) {
                    LOG_ERROR("Failed to create pipeline layout!");
                    return;
                }

                // 创建计算管线
                VkComputePipelineCreateInfo pipelineInfo = {};
                pipelineInfo.sType =
                    VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
                pipelineInfo.layout = State.CurrentPipelineLayout;
                pipelineInfo.stage.sType =
                    VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
                pipelineInfo.stage.module = State.ComputeShader;
                pipelineInfo.stage.pName = "main";

                if (State.CurrentPipeline != VK_NULL_HANDLE) {
                    vkDestroyPipeline(
                        Device->GetHandle(), State.CurrentPipeline, nullptr);
                }

                if (vkCreateComputePipelines(Device->GetHandle(),
                                             VK_NULL_HANDLE,
                                             1,
                                             &pipelineInfo,
                                             nullptr,
                                             &State.CurrentPipeline) !=
                    VK_SUCCESS) {
                    LOG_ERROR("Failed to create compute pipeline!");
                    return;
                }

                vkCmdBindPipeline(GetCommandBuffer(),
                                  VK_PIPELINE_BIND_POINT_COMPUTE,
                                  State.CurrentPipeline);
            }
        }

        void VulkanCommandList::SetConstantBuffer(uint32 slot,
                                                  IRHIBuffer* buffer) {
            if (!IsInRecordingState || !buffer) {
                return;
            }

            auto vulkanBuffer = dynamic_cast<VulkanBuffer*>(buffer);
            if (!vulkanBuffer) {
                LOG_ERROR("Invalid buffer type!");
                return;
            }

            // 创建描述符集布局（如果需要）
            if (State.DescriptorSetLayouts.empty()) {
                VkDescriptorSetLayoutBinding binding = {};
                binding.binding = slot;
                binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                binding.descriptorCount = 1;
                binding.stageFlags = VK_SHADER_STAGE_ALL;

                VkDescriptorSetLayoutCreateInfo layoutInfo = {};
                layoutInfo.sType =
                    VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                layoutInfo.bindingCount = 1;
                layoutInfo.pBindings = &binding;

                VkDescriptorSetLayout layout;
                if (vkCreateDescriptorSetLayout(
                        Device->GetHandle(), &layoutInfo, nullptr, &layout) !=
                    VK_SUCCESS) {
                    LOG_ERROR("Failed to create descriptor set layout!");
                    return;
                }
                State.DescriptorSetLayouts.push_back(layout);
            }

            // 创建描述符池（如果需要）
            if (State.DescriptorPool == VK_NULL_HANDLE) {
                VkDescriptorPoolSize poolSize = {};
                poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                poolSize.descriptorCount = 1000;  // 根据需要调整大小

                VkDescriptorPoolCreateInfo poolInfo = {};
                poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
                poolInfo.poolSizeCount = 1;
                poolInfo.pPoolSizes = &poolSize;
                poolInfo.maxSets = 1000;  // 根据需要调整大小

                if (vkCreateDescriptorPool(Device->GetHandle(),
                                           &poolInfo,
                                           nullptr,
                                           &State.DescriptorPool) !=
                    VK_SUCCESS) {
                    LOG_ERROR("Failed to create descriptor pool!");
                    return;
                }
            }

            // 分配描述符集
            VkDescriptorSetAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = State.DescriptorPool;
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &State.DescriptorSetLayouts[0];

            VkDescriptorSet descriptorSet;
            if (vkAllocateDescriptorSets(Device->GetHandle(),
                                         &allocInfo,
                                         &descriptorSet) != VK_SUCCESS) {
                LOG_ERROR("Failed to allocate descriptor set!");
                return;
            }

            // 更新描述符集
            VkDescriptorBufferInfo bufferInfo = {};
            bufferInfo.buffer = vulkanBuffer->GetHandle();
            bufferInfo.offset = 0;
            bufferInfo.range = vulkanBuffer->GetSize();

            VkWriteDescriptorSet descriptorWrite = {};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = descriptorSet;
            descriptorWrite.dstBinding = slot;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pBufferInfo = &bufferInfo;

            vkUpdateDescriptorSets(
                Device->GetHandle(), 1, &descriptorWrite, 0, nullptr);

            // 绑定描述符集
            if (State.CurrentPipelineLayout != VK_NULL_HANDLE) {
                vkCmdBindDescriptorSets(
                    GetCommandBuffer(),
                    VK_PIPELINE_BIND_POINT_GRAPHICS,  // 或
                                                      // VK_PIPELINE_BIND_POINT_COMPUTE
                    State.CurrentPipelineLayout,
                    0,
                    1,
                    &descriptorSet,
                    0,
                    nullptr);
            }

            // 存储描述符集以供后续使用
            State.CurrentDescriptorSets.push_back(descriptorSet);
        }
        void VulkanCommandList::SetShaderResource(uint32 slot,
                                                  IRHIShaderResourceView* srv) {
            if (!IsInRecordingState || !srv) {
                return;
            }

            auto vulkanSRV = dynamic_cast<VulkanShaderResourceView*>(srv);
            if (!vulkanSRV) {
                LOG_ERROR("Invalid shader resource view type!");
                return;
            }

            // 创建描述符集布局（如果需要）
            if (State.SRVDescriptorSetLayouts.empty()) {
                VkDescriptorSetLayoutBinding binding = {};
                binding.binding = slot;
                binding.descriptorType =
                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;  // 或其他类型，取决于资源
                binding.descriptorCount = 1;
                binding.stageFlags = VK_SHADER_STAGE_ALL;

                VkDescriptorSetLayoutCreateInfo layoutInfo = {};
                layoutInfo.sType =
                    VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                layoutInfo.bindingCount = 1;
                layoutInfo.pBindings = &binding;

                VkDescriptorSetLayout layout;
                if (vkCreateDescriptorSetLayout(
                        Device->GetHandle(), &layoutInfo, nullptr, &layout) !=
                    VK_SUCCESS) {
                    LOG_ERROR("Failed to create SRV descriptor set layout!");
                    return;
                }
                State.SRVDescriptorSetLayouts.push_back(layout);
            }

            // 创建描述符池（如果需要）
            if (State.SRVDescriptorPool == VK_NULL_HANDLE) {
                VkDescriptorPoolSize poolSize = {};
                poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                poolSize.descriptorCount = 1000;  // 根据需要调整大小

                VkDescriptorPoolCreateInfo poolInfo = {};
                poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
                poolInfo.poolSizeCount = 1;
                poolInfo.pPoolSizes = &poolSize;
                poolInfo.maxSets = 1000;  // 根据需要调整大小

                if (vkCreateDescriptorPool(Device->GetHandle(),
                                           &poolInfo,
                                           nullptr,
                                           &State.SRVDescriptorPool) !=
                    VK_SUCCESS) {
                    LOG_ERROR("Failed to create SRV descriptor pool!");
                    return;
                }
            }

            // 分配描述符集
            VkDescriptorSetAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = State.SRVDescriptorPool;
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &State.SRVDescriptorSetLayouts[0];

            VkDescriptorSet descriptorSet;
            if (vkAllocateDescriptorSets(Device->GetHandle(),
                                         &allocInfo,
                                         &descriptorSet) != VK_SUCCESS) {
                LOG_ERROR("Failed to allocate SRV descriptor set!");
                return;
            }

            // 更新描述符集
            VkDescriptorImageInfo imageInfo = {};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = vulkanSRV->GetHandle();
            imageInfo.sampler = VK_NULL_HANDLE;  // 如果需要采样器，在这里设置

            VkWriteDescriptorSet descriptorWrite = {};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = descriptorSet;
            descriptorWrite.dstBinding = slot;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType =
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(
                Device->GetHandle(), 1, &descriptorWrite, 0, nullptr);

            // 绑定描述符集
            if (State.CurrentPipelineLayout != VK_NULL_HANDLE) {
                vkCmdBindDescriptorSets(
                    GetCommandBuffer(),
                    VK_PIPELINE_BIND_POINT_GRAPHICS,  // 或
                                                      // VK_PIPELINE_BIND_POINT_COMPUTE
                    State.CurrentPipelineLayout,
                    1,  // 使用不同的set索引以区分不同类型的资源
                    1,
                    &descriptorSet,
                    0,
                    nullptr);
            }

            // 存储描述符集以供后续使用
            State.CurrentSRVDescriptorSets.push_back(descriptorSet);
        }
        void VulkanCommandList::SetUnorderedAccessView(
            uint32 slot, IRHIUnorderedAccessView* uav) {
            if (!IsInRecordingState || !uav) {
                return;
            }

            auto vulkanUAV = dynamic_cast<VulkanUnorderedAccessView*>(uav);
            if (!vulkanUAV) {
                LOG_ERROR("Invalid unordered access view type!");
                return;
            }

            // 创建描述符集布局（如果需要）
            if (State.UAVDescriptorSetLayouts.empty()) {
                VkDescriptorSetLayoutBinding binding = {};
                binding.binding = slot;
                binding.descriptorType =
                    VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;  // 或
                                                       // VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
                binding.descriptorCount = 1;
                binding.stageFlags = VK_SHADER_STAGE_ALL;

                VkDescriptorSetLayoutCreateInfo layoutInfo = {};
                layoutInfo.sType =
                    VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                layoutInfo.bindingCount = 1;
                layoutInfo.pBindings = &binding;

                VkDescriptorSetLayout layout;
                if (vkCreateDescriptorSetLayout(
                        Device->GetHandle(), &layoutInfo, nullptr, &layout) !=
                    VK_SUCCESS) {
                    LOG_ERROR("Failed to create UAV descriptor set layout!");
                    return;
                }
                State.UAVDescriptorSetLayouts.push_back(layout);
            }

            // 创建描述符池（如果需要）
            if (State.UAVDescriptorPool == VK_NULL_HANDLE) {
                VkDescriptorPoolSize poolSizes[2] = {};
                poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                poolSizes[0].descriptorCount = 500;
                poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                poolSizes[1].descriptorCount = 500;

                VkDescriptorPoolCreateInfo poolInfo = {};
                poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
                poolInfo.poolSizeCount = 2;
                poolInfo.pPoolSizes = poolSizes;
                poolInfo.maxSets = 1000;  // 根据需要调整大小

                if (vkCreateDescriptorPool(Device->GetHandle(),
                                           &poolInfo,
                                           nullptr,
                                           &State.UAVDescriptorPool) !=
                    VK_SUCCESS) {
                    LOG_ERROR("Failed to create UAV descriptor pool!");
                    return;
                }
            }

            // 分配描述符集
            VkDescriptorSetAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = State.UAVDescriptorPool;
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &State.UAVDescriptorSetLayouts[0];

            VkDescriptorSet descriptorSet;
            if (vkAllocateDescriptorSets(Device->GetHandle(),
                                         &allocInfo,
                                         &descriptorSet) != VK_SUCCESS) {
                LOG_ERROR("Failed to allocate UAV descriptor set!");
                return;
            }

            // 更新描述符集
            VkWriteDescriptorSet descriptorWrite = {};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = descriptorSet;
            descriptorWrite.dstBinding = slot;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorCount = 1;

            if (vulkanUAV->GetResourceType() == ERHIResourceDimension::Buffer) {
                // 缓冲区UAV
                VkDescriptorBufferInfo bufferInfo = {};
                bufferInfo.buffer = vulkanUAV->GetBufferHandle();
                bufferInfo.offset = 0;
                bufferInfo.range = VK_WHOLE_SIZE;

                descriptorWrite.descriptorType =
                    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                descriptorWrite.pBufferInfo = &bufferInfo;
            } else {
                // 图像UAV
                VkDescriptorImageInfo imageInfo = {};
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                imageInfo.imageView = vulkanUAV->GetHandle();
                imageInfo.sampler = VK_NULL_HANDLE;

                descriptorWrite.descriptorType =
                    VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                descriptorWrite.pImageInfo = &imageInfo;
            }

            vkUpdateDescriptorSets(
                Device->GetHandle(), 1, &descriptorWrite, 0, nullptr);

            // 绑定描述符集
            if (State.CurrentPipelineLayout != VK_NULL_HANDLE) {
                vkCmdBindDescriptorSets(
                    GetCommandBuffer(),
                    VK_PIPELINE_BIND_POINT_GRAPHICS,  // 或
                                                      // VK_PIPELINE_BIND_POINT_COMPUTE
                    State.CurrentPipelineLayout,
                    2,  // 使用不同的set索引以区分不同类型的资源
                    1,
                    &descriptorSet,
                    0,
                    nullptr);
            }

            // 存储描述符集以供后续使用
            State.CurrentUAVDescriptorSets.push_back(descriptorSet);

            // 如果是图像UAV，可能需要进行布局转换
            if (vulkanUAV->GetResourceType() != ERHIResourceDimension::Buffer) {
                VkImageMemoryBarrier barrier = {};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.image = vulkanUAV->GetImageHandle();
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseMipLevel = 0;
                barrier.subresourceRange.levelCount = 1;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = 1;
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

                vkCmdPipelineBarrier(GetCommandBuffer(),
                                     VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                     0,
                                     0,
                                     nullptr,
                                     0,
                                     nullptr,
                                     1,
                                     &barrier);
            }
        }

    }  // namespace RHI
}  // namespace Engine
