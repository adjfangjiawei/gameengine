
#pragma once

#include <array>
#include <memory>

#include "RHIDevice.h"

namespace Engine {
    namespace RHI {

        // 渲染目标绑定
        struct RenderTargetBinding {
            IRHIRenderTargetView* RenderTarget = nullptr;
            IRHIDepthStencilView* DepthStencil = nullptr;
            ERHIResourceState RenderTargetState =
                ERHIResourceState::RenderTarget;
            ERHIResourceState DepthStencilState = ERHIResourceState::DepthWrite;
        };

        // 着色器绑定
        struct ShaderBinding {
            IRHIShader* Shader = nullptr;
            std::vector<IRHIBuffer*> ConstantBuffers;
            std::vector<IRHIShaderResourceView*> ShaderResources;
            std::vector<IRHISamplerState*> Samplers;
            std::vector<IRHIUnorderedAccessView*> UnorderedAccesses;
        };

        // 顶点流绑定
        struct VertexStreamBinding {
            std::vector<VertexBufferView> VertexBuffers;
            IndexBufferView IndexBuffer;
            EPrimitiveType PrimitiveType = EPrimitiveType::Triangles;
        };

        // RHI上下文接口
        class IRHIContext {
          public:
            virtual ~IRHIContext() = default;

            // 设备访问
            virtual IRHIDevice* GetDevice() = 0;
            virtual const IRHIDevice* GetDevice() const = 0;

            // 命令列表管理
            virtual IRHICommandList* GetCurrentCommandList() = 0;
            virtual void BeginFrame() = 0;
            virtual void EndFrame() = 0;
            virtual void FlushCommandList() = 0;

            // 渲染目标操作
            virtual void SetRenderTargets(
                const RenderTargetBinding& binding) = 0;
            virtual void ClearRenderTarget(IRHIRenderTargetView* renderTarget,
                                           const float* clearColor) = 0;
            virtual void ClearDepthStencil(IRHIDepthStencilView* depthStencil,
                                           float depth = 1.0f,
                                           uint8 stencil = 0) = 0;

            // 视口和裁剪矩形
            virtual void SetViewport(const Viewport& viewport) = 0;
            virtual void SetScissorRect(const ScissorRect& scissor) = 0;

            // 着色器绑定
            virtual void SetVertexShader(const ShaderBinding& binding) = 0;
            virtual void SetPixelShader(const ShaderBinding& binding) = 0;
            virtual void SetGeometryShader(const ShaderBinding& binding) = 0;
            virtual void SetComputeShader(const ShaderBinding& binding) = 0;
            virtual void SetHullShader(const ShaderBinding& binding) = 0;
            virtual void SetDomainShader(const ShaderBinding& binding) = 0;

            // 顶点流绑定
            virtual void SetVertexBuffers(
                const VertexStreamBinding& binding) = 0;

            // 渲染状态
            virtual void SetBlendState(const BlendDesc& desc) = 0;
            virtual void SetRasterizerState(const RasterizerDesc& desc) = 0;
            virtual void SetDepthStencilState(const DepthStencilDesc& desc) = 0;

            // 绘制调用
            virtual void Draw(uint32 vertexCount,
                              uint32 startVertexLocation = 0) = 0;
            virtual void DrawIndexed(uint32 indexCount,
                                     uint32 startIndexLocation = 0,
                                     int32 baseVertexLocation = 0) = 0;
            virtual void DrawInstanced(uint32 vertexCountPerInstance,
                                       uint32 instanceCount,
                                       uint32 startVertexLocation = 0,
                                       uint32 startInstanceLocation = 0) = 0;
            virtual void DrawIndexedInstanced(
                uint32 indexCountPerInstance,
                uint32 instanceCount,
                uint32 startIndexLocation = 0,
                int32 baseVertexLocation = 0,
                uint32 startInstanceLocation = 0) = 0;

            // 计算调度
            virtual void Dispatch(uint32 threadGroupCountX,
                                  uint32 threadGroupCountY,
                                  uint32 threadGroupCountZ) = 0;

            // 资源状态转换
            virtual void TransitionResource(IRHIResource* resource,
                                            ERHIResourceState newState) = 0;

            // 资源复制
            virtual void CopyResource(IRHIResource* dest,
                                      IRHIResource* source) = 0;
            virtual void CopyBufferRegion(IRHIBuffer* dest,
                                          uint64 destOffset,
                                          IRHIBuffer* source,
                                          uint64 sourceOffset,
                                          uint64 size) = 0;
            virtual void CopyTextureRegion(IRHITexture* dest,
                                           uint32 destX,
                                           uint32 destY,
                                           uint32 destZ,
                                           IRHITexture* source,
                                           uint32 sourceSubresource) = 0;

            // 调试支持
            virtual void BeginEvent(const char* name) = 0;
            virtual void EndEvent() = 0;
            virtual void SetMarker(const char* name) = 0;

            // 状态查询
            virtual void GetViewport(Viewport& viewport) const = 0;
            virtual void GetScissorRect(ScissorRect& scissor) const = 0;
            virtual const BlendDesc& GetBlendState() const = 0;
            virtual const RasterizerDesc& GetRasterizerState() const = 0;
            virtual const DepthStencilDesc& GetDepthStencilState() const = 0;

            // 默认状态获取
            virtual IRHISamplerState* GetDefaultSamplerState() = 0;
            virtual const BlendDesc& GetDefaultBlendDesc() const = 0;
            virtual const RasterizerDesc& GetDefaultRasterizerDesc() const = 0;
            virtual const DepthStencilDesc& GetDefaultDepthStencilDesc()
                const = 0;

          protected:
            // 状态缓存
            virtual void InvalidateState() = 0;
            virtual void ApplyState() = 0;
        };

        // 智能指针类型定义
        using RHIContextPtr = std::shared_ptr<IRHIContext>;

        // 创建RHI上下文
        RHIContextPtr CreateRHIContext(IRHIDevice* device);

    }  // namespace RHI
}  // namespace Engine
