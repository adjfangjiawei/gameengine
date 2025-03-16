
#pragma once

#include <vector>

#include "VulkanRHI.h"
#include "VulkanResources.h"

namespace Engine {
    namespace RHI {

        // 命令缓冲区包装器
        class VulkanCommandBuffer {
          public:
            VulkanCommandBuffer(
                VulkanDevice* device,
                VkCommandPool commandPool,
                VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
            ~VulkanCommandBuffer();

            // 开始和结束记录
            bool Begin(VkCommandBufferUsageFlags flags = 0);
            bool End();
            void Reset();

            // 获取原生句柄
            VkCommandBuffer GetHandle() const { return CommandBuffer; }

          private:
            VulkanDevice* Device;
            VkCommandPool CommandPool;
            VkCommandBuffer CommandBuffer;
        };

        // Vulkan命令列表实现
        class VulkanCommandList : public IRHICommandList {
          public:
            VulkanCommandList(VulkanDevice* device, ECommandListType type);
            virtual ~VulkanCommandList();

            // IRHICommandList接口实现
            virtual void Reset(IRHICommandAllocator* allocator) override;
            virtual void Close() override;
            virtual void BeginEvent(const char* name) override;
            virtual void EndEvent() override;

            virtual void ResourceBarrier(IRHIResource* resource,
                                         ERHIResourceState before,
                                         ERHIResourceState after) override;

            virtual void SetRenderTargets(
                uint32 numRenderTargets,
                IRHIRenderTargetView* const* renderTargets,
                IRHIDepthStencilView* depthStencil) override;

            virtual void ClearRenderTargetView(
                IRHIRenderTargetView* renderTarget,
                const float* clearColor) override;

            virtual void ClearDepthStencilView(
                IRHIDepthStencilView* depthStencil,
                float depth,
                uint8 stencil) override;

            virtual void SetViewport(const Viewport& viewport) override;
            virtual void SetScissorRect(const ScissorRect& scissor) override;
            virtual void SetPrimitiveTopology(
                EPrimitiveType primitiveType) override;

            virtual void SetVertexBuffers(
                uint32 startSlot,
                uint32 numBuffers,
                const VertexBufferView* views) override;

            virtual void SetIndexBuffer(const IndexBufferView& view) override;

            virtual void SetShader(EShaderType type,
                                   IRHIShader* shader) override;

            virtual void SetConstantBuffer(uint32 slot,
                                           IRHIBuffer* buffer) override;

            virtual void SetShaderResource(
                uint32 slot, IRHIShaderResourceView* srv) override;

            virtual void SetUnorderedAccessView(
                uint32 slot, IRHIUnorderedAccessView* uav) override;

            virtual void Draw(uint32 vertexCount,
                              uint32 startVertexLocation) override;

            virtual void DrawIndexed(uint32 indexCount,
                                     uint32 startIndexLocation,
                                     int32 baseVertexLocation) override;

            virtual void DrawInstanced(uint32 vertexCountPerInstance,
                                       uint32 instanceCount,
                                       uint32 startVertexLocation,
                                       uint32 startInstanceLocation) override;

            virtual void DrawIndexedInstanced(
                uint32 indexCountPerInstance,
                uint32 instanceCount,
                uint32 startIndexLocation,
                int32 baseVertexLocation,
                uint32 startInstanceLocation) override;

            virtual void Dispatch(uint32 threadGroupCountX,
                                  uint32 threadGroupCountY,
                                  uint32 threadGroupCountZ) override;

            virtual void CopyBuffer(IRHIBuffer* dest,
                                    uint64 destOffset,
                                    IRHIBuffer* source,
                                    uint64 sourceOffset,
                                    uint64 size) override;

            virtual void CopyTexture(IRHITexture* dest,
                                     uint32 destSubresource,
                                     uint32 destX,
                                     uint32 destY,
                                     uint32 destZ,
                                     IRHITexture* source,
                                     uint32 sourceSubresource) override;

            virtual ECommandListType GetType() const override { return Type; }

            // Vulkan特定方法
            VkCommandBuffer GetCommandBuffer() const;
            bool IsRecording() const { return IsInRecordingState; }

          private:
            void BeginRenderPass();
            void EndRenderPass();
            void EnsureCommandBuffer();
            void TransitionImageLayout(VkImage image,
                                       VkImageLayout oldLayout,
                                       VkImageLayout newLayout);

            VulkanDevice* Device;
            ECommandListType Type;
            VulkanCommandBuffer* CurrentCommandBuffer;
            bool IsInRecordingState;
            bool IsInRenderPass;

            // 当前状态缓存
            struct {
                std::vector<VkImageView> RenderTargetViews;
                VkImageView DepthStencilView;
                VkRenderPass CurrentRenderPass;
                VkFramebuffer CurrentFramebuffer;
                VkPipeline CurrentPipeline;
                VkPipelineLayout CurrentPipelineLayout;
                std::vector<VkDescriptorSet> CurrentDescriptorSets;
            } State;
        };

        // Vulkan命令分配器实现
        class VulkanCommandAllocator : public IRHICommandAllocator {
          public:
            VulkanCommandAllocator(VulkanDevice* device, ECommandListType type);
            virtual ~VulkanCommandAllocator();

            // IRHICommandAllocator接口实现
            virtual void Reset() override;
            virtual ECommandListType GetType() const override { return Type; }

            // Vulkan特定方法
            VkCommandPool GetCommandPool() const { return CommandPool; }

          private:
            VulkanDevice* Device;
            ECommandListType Type;
            VkCommandPool CommandPool;
        };

    }  // namespace RHI
}  // namespace Engine
