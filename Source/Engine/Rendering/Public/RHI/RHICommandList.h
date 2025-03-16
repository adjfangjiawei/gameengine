
#pragma once

#include <memory>

#include "RHIResources.h"

namespace Engine {
    namespace RHI {

        // 前向声明
        class IRHICommandAllocator;

        // 视口定义
        struct Viewport {
            float TopLeftX = 0.0f;
            float TopLeftY = 0.0f;
            float Width = 0.0f;
            float Height = 0.0f;
            float MinDepth = 0.0f;
            float MaxDepth = 1.0f;
        };

        // 裁剪矩形
        struct ScissorRect {
            int32 Left = 0;
            int32 Top = 0;
            int32 Right = 0;
            int32 Bottom = 0;
        };

        // 顶点缓冲区视图
        struct VertexBufferView {
            IRHIBuffer* Buffer = nullptr;
            uint64 Offset = 0;
            uint32 Stride = 0;
            uint32 Size = 0;
        };

        // 索引缓冲区视图
        struct IndexBufferView {
            IRHIBuffer* Buffer = nullptr;
            uint64 Offset = 0;
            uint32 Size = 0;
            EPixelFormat Format = EPixelFormat::R32_FLOAT;
        };

        // 命令列表类型
        enum class ECommandListType : uint8 {
            Direct,   // 可以执行任何类型的命令
            Bundle,   // 预录制的命令包
            Compute,  // 仅计算命令
            Copy,     // 仅复制命令
        };

        // 命令分配器接口
        class IRHICommandAllocator {
          public:
            virtual ~IRHICommandAllocator() = default;

            // 重置命令分配器
            virtual void Reset() = 0;

            // 获取分配器类型
            virtual ECommandListType GetType() const = 0;
        };

        // 命令列表接口
        class IRHICommandList {
          public:
            virtual ~IRHICommandList() = default;

            // 基础操作
            virtual void Reset(IRHICommandAllocator* allocator) = 0;
            virtual void Close() = 0;
            virtual void BeginEvent(const char* name) = 0;
            virtual void EndEvent() = 0;

            // 资源状态转换
            virtual void ResourceBarrier(IRHIResource* resource,
                                         ERHIResourceState before,
                                         ERHIResourceState after) = 0;

            // 设置渲染目标
            virtual void SetRenderTargets(
                uint32 numRenderTargets,
                IRHIRenderTargetView* const* renderTargets,
                IRHIDepthStencilView* depthStencil) = 0;

            // 清除渲染目标
            virtual void ClearRenderTargetView(
                IRHIRenderTargetView* renderTarget,
                const float* clearColor) = 0;

            // 清除深度模板
            virtual void ClearDepthStencilView(
                IRHIDepthStencilView* depthStencil,
                float depth,
                uint8 stencil) = 0;

            // 设置视口
            virtual void SetViewport(const Viewport& viewport) = 0;

            // 设置裁剪矩形
            virtual void SetScissorRect(const ScissorRect& scissor) = 0;

            // 设置图元拓扑
            virtual void SetPrimitiveTopology(EPrimitiveType primitiveType) = 0;

            // 设置顶点缓冲区
            virtual void SetVertexBuffers(uint32 startSlot,
                                          uint32 numBuffers,
                                          const VertexBufferView* views) = 0;

            // 设置索引缓冲区
            virtual void SetIndexBuffer(const IndexBufferView& view) = 0;

            // 设置着色器
            virtual void SetShader(EShaderType type, IRHIShader* shader) = 0;

            // 设置常量缓冲区
            virtual void SetConstantBuffer(uint32 slot, IRHIBuffer* buffer) = 0;

            // 设置着色器资源
            virtual void SetShaderResource(uint32 slot,
                                           IRHIShaderResourceView* srv) = 0;

            // 设置无序访问视图
            virtual void SetUnorderedAccessView(
                uint32 slot, IRHIUnorderedAccessView* uav) = 0;

            // 绘制调用
            virtual void Draw(uint32 vertexCount,
                              uint32 startVertexLocation) = 0;

            virtual void DrawIndexed(uint32 indexCount,
                                     uint32 startIndexLocation,
                                     int32 baseVertexLocation) = 0;

            virtual void DrawInstanced(uint32 vertexCountPerInstance,
                                       uint32 instanceCount,
                                       uint32 startVertexLocation,
                                       uint32 startInstanceLocation) = 0;

            virtual void DrawIndexedInstanced(uint32 indexCountPerInstance,
                                              uint32 instanceCount,
                                              uint32 startIndexLocation,
                                              int32 baseVertexLocation,
                                              uint32 startInstanceLocation) = 0;

            // 计算调度
            virtual void Dispatch(uint32 threadGroupCountX,
                                  uint32 threadGroupCountY,
                                  uint32 threadGroupCountZ) = 0;

            // 复制操作
            virtual void CopyBuffer(IRHIBuffer* dest,
                                    uint64 destOffset,
                                    IRHIBuffer* source,
                                    uint64 sourceOffset,
                                    uint64 size) = 0;

            virtual void CopyTexture(IRHITexture* dest,
                                     uint32 destSubresource,
                                     uint32 destX,
                                     uint32 destY,
                                     uint32 destZ,
                                     IRHITexture* source,
                                     uint32 sourceSubresource) = 0;

            // 获取命令列表类型
            virtual ECommandListType GetType() const = 0;
        };

        // 智能指针类型定义
        using RHICommandAllocatorPtr = std::shared_ptr<IRHICommandAllocator>;
        using RHICommandListPtr = std::shared_ptr<IRHICommandList>;

    }  // namespace RHI
}  // namespace Engine
