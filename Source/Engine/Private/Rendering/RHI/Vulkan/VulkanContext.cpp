
#include "Core/Log/LogSystem.h"
#include "VulkanRHI.h"

namespace Engine {
    namespace RHI {

        class VulkanContextImpl : public IRHIContext {
          public:
            VulkanContextImpl(IRHIDevice* device)
                : Device(device), CurrentCommandList(nullptr), FrameIndex(0) {
                // 创建命令分配器
                CommandAllocators[0] =
                    Device->CreateCommandAllocator(ECommandListType::Direct);
                CommandAllocators[1] =
                    Device->CreateCommandAllocator(ECommandListType::Direct);

                // 创建默认状态对象
                CreateDefaultStates();
            }

            virtual ~VulkanContextImpl() {
                CommandAllocators[0].reset();
                CommandAllocators[1].reset();
            }

            // 设备访问
            virtual IRHIDevice* GetDevice() override { return Device; }
            virtual const IRHIDevice* GetDevice() const override {
                return Device;
            }

            // 命令列表管理
            virtual IRHICommandList* GetCurrentCommandList() override {
                return CurrentCommandList.get();
            }

            virtual void BeginFrame() override {
                auto& allocator = CommandAllocators[FrameIndex];
                allocator->Reset();

                CurrentCommandList = Device->CreateCommandList(
                    ECommandListType::Direct, allocator.get());
                InvalidateState();

                FrameIndex = (FrameIndex + 1) % 2;
            }

            virtual void EndFrame() override {
                if (CurrentCommandList) {
                    CurrentCommandList->Close();
                    Device->SubmitCommandLists(1, &CurrentCommandList.get());
                }
            }

            virtual void FlushCommandList() override {
                if (CurrentCommandList) {
                    CurrentCommandList->Close();
                    Device->SubmitCommandLists(1, &CurrentCommandList.get());
                    Device->WaitForGPU();
                }
            }

            // 渲染目标操作
            virtual void SetRenderTargets(
                const RenderTargetBinding& binding) override {
                if (!CurrentCommandList) return;

                CurrentCommandList->SetRenderTargets(
                    1, &binding.RenderTarget, binding.DepthStencil);

                // 记录当前绑定
                CurrentRenderTarget = binding;
            }

            virtual void ClearRenderTarget(IRHIRenderTargetView* renderTarget,
                                           const float* clearColor) override {
                if (!CurrentCommandList) return;

                CurrentCommandList->ClearRenderTargetView(renderTarget,
                                                          clearColor);
            }

            virtual void ClearDepthStencil(IRHIDepthStencilView* depthStencil,
                                           float depth,
                                           uint8 stencil) override {
                if (!CurrentCommandList) return;

                CurrentCommandList->ClearDepthStencilView(
                    depthStencil, depth, stencil);
            }

            // 视口和裁剪矩形
            virtual void SetViewport(const Viewport& viewport) override {
                if (!CurrentCommandList) return;

                CurrentCommandList->SetViewport(viewport);
                CurrentViewport = viewport;
            }

            virtual void SetScissorRect(const ScissorRect& scissor) override {
                if (!CurrentCommandList) return;

                CurrentCommandList->SetScissorRect(scissor);
                CurrentScissor = scissor;
            }

            // 着色器绑定
            virtual void SetVertexShader(
                const ShaderBinding& binding) override {
                if (!CurrentCommandList) return;

                CurrentCommandList->SetShader(EShaderType::Vertex,
                                              binding.Shader);
                ApplyShaderResources(binding);
            }

            virtual void SetPixelShader(const ShaderBinding& binding) override {
                if (!CurrentCommandList) return;

                CurrentCommandList->SetShader(EShaderType::Pixel,
                                              binding.Shader);
                ApplyShaderResources(binding);
            }

            virtual void SetGeometryShader(
                const ShaderBinding& binding) override {
                if (!CurrentCommandList) return;

                CurrentCommandList->SetShader(EShaderType::Geometry,
                                              binding.Shader);
                ApplyShaderResources(binding);
            }

            virtual void SetComputeShader(
                const ShaderBinding& binding) override {
                if (!CurrentCommandList) return;

                CurrentCommandList->SetShader(EShaderType::Compute,
                                              binding.Shader);
                ApplyShaderResources(binding);
            }

            virtual void SetHullShader(const ShaderBinding& binding) override {
                if (!CurrentCommandList) return;

                CurrentCommandList->SetShader(EShaderType::Hull,
                                              binding.Shader);
                ApplyShaderResources(binding);
            }

            virtual void SetDomainShader(
                const ShaderBinding& binding) override {
                if (!CurrentCommandList) return;

                CurrentCommandList->SetShader(EShaderType::Domain,
                                              binding.Shader);
                ApplyShaderResources(binding);
            }

            // 顶点流绑定
            virtual void SetVertexBuffers(
                const VertexStreamBinding& binding) override {
                if (!CurrentCommandList) return;

                CurrentCommandList->SetVertexBuffers(
                    0,
                    static_cast<uint32>(binding.VertexBuffers.size()),
                    binding.VertexBuffers.data());

                if (binding.IndexBuffer.Buffer) {
                    CurrentCommandList->SetIndexBuffer(binding.IndexBuffer);
                }

                CurrentCommandList->SetPrimitiveTopology(binding.PrimitiveType);
            }

            // 渲染状态
            virtual void SetBlendState(const BlendDesc& desc) override {
                CurrentBlendDesc = desc;
                // TODO: 应用混合状态
            }

            virtual void SetRasterizerState(
                const RasterizerDesc& desc) override {
                CurrentRasterizerDesc = desc;
                // TODO: 应用光栅化状态
            }

            virtual void SetDepthStencilState(
                const DepthStencilDesc& desc) override {
                CurrentDepthStencilDesc = desc;
                // TODO: 应用深度模板状态
            }

            // 绘制调用
            virtual void Draw(uint32 vertexCount,
                              uint32 startVertexLocation) override {
                if (!CurrentCommandList) return;

                ApplyState();
                CurrentCommandList->Draw(vertexCount, startVertexLocation);
            }

            virtual void DrawIndexed(uint32 indexCount,
                                     uint32 startIndexLocation,
                                     int32 baseVertexLocation) override {
                if (!CurrentCommandList) return;

                ApplyState();
                CurrentCommandList->DrawIndexed(
                    indexCount, startIndexLocation, baseVertexLocation);
            }

            virtual void DrawInstanced(uint32 vertexCountPerInstance,
                                       uint32 instanceCount,
                                       uint32 startVertexLocation,
                                       uint32 startInstanceLocation) override {
                if (!CurrentCommandList) return;

                ApplyState();
                CurrentCommandList->DrawInstanced(vertexCountPerInstance,
                                                  instanceCount,
                                                  startVertexLocation,
                                                  startInstanceLocation);
            }

            virtual void DrawIndexedInstanced(
                uint32 indexCountPerInstance,
                uint32 instanceCount,
                uint32 startIndexLocation,
                int32 baseVertexLocation,
                uint32 startInstanceLocation) override {
                if (!CurrentCommandList) return;

                ApplyState();
                CurrentCommandList->DrawIndexedInstanced(indexCountPerInstance,
                                                         instanceCount,
                                                         startIndexLocation,
                                                         baseVertexLocation,
                                                         startInstanceLocation);
            }

            // 计算调度
            virtual void Dispatch(uint32 threadGroupCountX,
                                  uint32 threadGroupCountY,
                                  uint32 threadGroupCountZ) override {
                if (!CurrentCommandList) return;

                ApplyState();
                CurrentCommandList->Dispatch(
                    threadGroupCountX, threadGroupCountY, threadGroupCountZ);
            }

            // 资源状态转换
            virtual void TransitionResource(
                IRHIResource* resource, ERHIResourceState newState) override {
                if (!CurrentCommandList || !resource) return;

                auto currentState = resource->GetCurrentState();
                if (currentState != newState) {
                    CurrentCommandList->ResourceBarrier(
                        resource, currentState, newState);
                }
            }

            // 资源复制
            virtual void CopyResource(IRHIResource* dest,
                                      IRHIResource* source) override {
                if (!CurrentCommandList || !dest || !source) return;

                if (dest->GetResourceDimension() ==
                        ERHIResourceDimension::Buffer &&
                    source->GetResourceDimension() ==
                        ERHIResourceDimension::Buffer) {
                    auto srcBuffer = static_cast<IRHIBuffer*>(source);
                    CurrentCommandList->CopyBuffer(
                        static_cast<IRHIBuffer*>(dest),
                        0,
                        srcBuffer,
                        0,
                        srcBuffer->GetSize());
                } else if (dest->GetResourceDimension() !=
                               ERHIResourceDimension::Buffer &&
                           source->GetResourceDimension() !=
                               ERHIResourceDimension::Buffer) {
                    CurrentCommandList->CopyTexture(
                        static_cast<IRHITexture*>(dest),
                        0,
                        0,
                        0,
                        0,
                        static_cast<IRHITexture*>(source),
                        0);
                }
            }

            virtual void CopyBufferRegion(IRHIBuffer* dest,
                                          uint64 destOffset,
                                          IRHIBuffer* source,
                                          uint64 sourceOffset,
                                          uint64 size) override {
                if (!CurrentCommandList) return;

                CurrentCommandList->CopyBuffer(
                    dest, destOffset, source, sourceOffset, size);
            }

            virtual void CopyTextureRegion(IRHITexture* dest,
                                           uint32 destX,
                                           uint32 destY,
                                           uint32 destZ,
                                           IRHITexture* source,
                                           uint32 sourceSubresource) override {
                if (!CurrentCommandList) return;

                CurrentCommandList->CopyTexture(
                    dest, 0, destX, destY, destZ, source, sourceSubresource);
            }

            // 调试支持
            virtual void BeginEvent(const char* name) override {
                if (!CurrentCommandList) return;

                CurrentCommandList->BeginEvent(name);
            }

            virtual void EndEvent() override {
                if (!CurrentCommandList) return;

                CurrentCommandList->EndEvent();
            }

            virtual void SetMarker(const char* name) override {
                if (!CurrentCommandList) return;

                CurrentCommandList->BeginEvent(name);
                CurrentCommandList->EndEvent();
            }

            // 状态查询
            virtual void GetViewport(Viewport& viewport) const override {
                viewport = CurrentViewport;
            }

            virtual void GetScissorRect(ScissorRect& scissor) const override {
                scissor = CurrentScissor;
            }

            virtual const BlendDesc& GetBlendState() const override {
                return CurrentBlendDesc;
            }

            virtual const RasterizerDesc& GetRasterizerState() const override {
                return CurrentRasterizerDesc;
            }

            virtual const DepthStencilDesc& GetDepthStencilState()
                const override {
                return CurrentDepthStencilDesc;
            }

            // 默认状态获取
            virtual IRHISamplerState* GetDefaultSamplerState() override {
                return DefaultSamplerState.get();
            }

            virtual const BlendDesc& GetDefaultBlendDesc() const override {
                return DefaultBlendDesc;
            }

            virtual const RasterizerDesc& GetDefaultRasterizerDesc()
                const override {
                return DefaultRasterizerDesc;
            }

            virtual const DepthStencilDesc& GetDefaultDepthStencilDesc()
                const override {
                return DefaultDepthStencilDesc;
            }

          protected:
            // 状态管理
            virtual void InvalidateState() override {
                // 重置所有状态缓存
                CurrentRenderTarget = RenderTargetBinding();
                CurrentViewport = Viewport();
                CurrentScissor = ScissorRect();
                CurrentBlendDesc = DefaultBlendDesc;
                CurrentRasterizerDesc = DefaultRasterizerDesc;
                CurrentDepthStencilDesc = DefaultDepthStencilDesc;
            }

            virtual void ApplyState() override {
                // TODO: 应用所有缓存的状态
            }

          private:
            void CreateDefaultStates() {
                // 创建默认采样器状态
                SamplerDesc samplerDesc;
                samplerDesc.Filter = ESamplerFilter::MinMagMipLinear;
                samplerDesc.AddressU = ESamplerAddressMode::Wrap;
                samplerDesc.AddressV = ESamplerAddressMode::Wrap;
                samplerDesc.AddressW = ESamplerAddressMode::Wrap;
                DefaultSamplerState.reset(
                    Device->CreateSamplerState(samplerDesc));

                // 设置默认混合状态
                DefaultBlendDesc.AlphaToCoverageEnable = false;
                DefaultBlendDesc.IndependentBlendEnable = false;
                DefaultBlendDesc.RenderTarget[0].BlendEnable = false;
                DefaultBlendDesc.RenderTarget[0].SrcBlend = EBlendFactor::One;
                DefaultBlendDesc.RenderTarget[0].DestBlend = EBlendFactor::Zero;
                DefaultBlendDesc.RenderTarget[0].BlendOp = EBlendOp::Add;
                DefaultBlendDesc.RenderTarget[0].SrcBlendAlpha =
                    EBlendFactor::One;
                DefaultBlendDesc.RenderTarget[0].DestBlendAlpha =
                    EBlendFactor::Zero;
                DefaultBlendDesc.RenderTarget[0].BlendOpAlpha = EBlendOp::Add;
                DefaultBlendDesc.RenderTarget[0].RenderTargetWriteMask = 0x0F;

                // 设置默认光栅化状态
                DefaultRasterizerDesc.FillMode = EFillMode::Solid;
                DefaultRasterizerDesc.CullMode = ECullMode::Back;
                DefaultRasterizerDesc.FrontCounterClockwise = false;
                DefaultRasterizerDesc.DepthBias = 0;
                DefaultRasterizerDesc.DepthBiasClamp = 0.0f;
                DefaultRasterizerDesc.SlopeScaledDepthBias = 0.0f;
                DefaultRasterizerDesc.DepthClipEnable = true;
                DefaultRasterizerDesc.MultisampleEnable = false;
                DefaultRasterizerDesc.AntialiasedLineEnable = false;

                // 设置默认深度模板状态
                DefaultDepthStencilDesc.DepthEnable = true;
                DefaultDepthStencilDesc.DepthWriteMask = true;
                DefaultDepthStencilDesc.DepthFunc = ECompareFunction::Less;
                DefaultDepthStencilDesc.StencilEnable = false;
                DefaultDepthStencilDesc.StencilReadMask = 0xFF;
                DefaultDepthStencilDesc.StencilWriteMask = 0xFF;

                // 应用默认状态
                CurrentBlendDesc = DefaultBlendDesc;
                CurrentRasterizerDesc = DefaultRasterizerDesc;
                CurrentDepthStencilDesc = DefaultDepthStencilDesc;
            }

            void ApplyShaderResources(const ShaderBinding& binding) {
                for (size_t i = 0; i < binding.ConstantBuffers.size(); ++i) {
                    CurrentCommandList->SetConstantBuffer(
                        i, binding.ConstantBuffers[i]);
                }

                for (size_t i = 0; i < binding.ShaderResources.size(); ++i) {
                    CurrentCommandList->SetShaderResource(
                        i, binding.ShaderResources[i]);
                }

                for (size_t i = 0; i < binding.UnorderedAccesses.size(); ++i) {
                    CurrentCommandList->SetUnorderedAccessView(
                        i, binding.UnorderedAccesses[i]);
                }
            }

            IRHIDevice* Device;
            RHICommandAllocatorPtr CommandAllocators[2];
            RHICommandListPtr CurrentCommandList;
            uint32 FrameIndex;

            // 状态缓存
            RenderTargetBinding CurrentRenderTarget;
            Viewport CurrentViewport;
            ScissorRect CurrentScissor;
            BlendDesc CurrentBlendDesc;
            RasterizerDesc CurrentRasterizerDesc;
            DepthStencilDesc CurrentDepthStencilDesc;

            // 默认状态
            std::unique_ptr<IRHISamplerState> DefaultSamplerState;
            BlendDesc DefaultBlendDesc;
            RasterizerDesc DefaultRasterizerDesc;
            DepthStencilDesc DefaultDepthStencilDesc;
        };

        // 创建RHI上下文
        RHIContextPtr CreateRHIContext(IRHIDevice* device) {
            return std::make_shared<VulkanContextImpl>(device);
        }

    }  // namespace RHI
}  // namespace Engine
