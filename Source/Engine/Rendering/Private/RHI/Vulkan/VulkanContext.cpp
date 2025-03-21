
#include "Core//Public/Log/LogSystem.h"
#include "VulkanCommandList.h"
#include "VulkanRHI.h"
#include "VulkanTypeOperators.h"

namespace Engine {
    namespace RHI {

        class VulkanContextImpl : public IRHIContext {
          public:
            VulkanContextImpl(IRHIDevice* device)
                : Device(RHIDevicePtr(device)),
                  CurrentCommandList(nullptr),
                  FrameIndex(0) {
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
            virtual IRHIDevice* GetDevice() override { return Device.get(); }
            virtual const IRHIDevice* GetDevice() const override {
                return Device.get();
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
                    IRHICommandList* cmdList = CurrentCommandList.get();
                    Device->SubmitCommandLists(1, &cmdList);
                }
            }

            virtual void FlushCommandList() override {
                if (CurrentCommandList) {
                    CurrentCommandList->Close();
                    IRHICommandList* cmdList = CurrentCommandList.get();
                    Device->SubmitCommandLists(1, &cmdList);
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

                if (!CurrentCommandList) return;

                // 获取Vulkan命令列表
                auto* vulkanCmdList =
                    static_cast<VulkanCommandList*>(CurrentCommandList.get());

                // 创建颜色混合附件状态
                std::vector<VkPipelineColorBlendAttachmentState>
                    colorBlendAttachments;
                for (uint32_t i = 0; i < 8; ++i) {
                    const auto& rt = desc.RenderTarget[i];
                    VkPipelineColorBlendAttachmentState attachmentState = {};

                    attachmentState.blendEnable = rt.BlendEnable;
                    attachmentState.srcColorBlendFactor =
                        ConvertToVkBlendFactor(rt.SrcBlend);
                    attachmentState.dstColorBlendFactor =
                        ConvertToVkBlendFactor(rt.DestBlend);
                    attachmentState.colorBlendOp =
                        ConvertToVkBlendOp(rt.BlendOp);
                    attachmentState.srcAlphaBlendFactor =
                        ConvertToVkBlendFactor(rt.SrcBlendAlpha);
                    attachmentState.dstAlphaBlendFactor =
                        ConvertToVkBlendFactor(rt.DestBlendAlpha);
                    attachmentState.alphaBlendOp =
                        ConvertToVkBlendOp(rt.BlendOpAlpha);
                    attachmentState.colorWriteMask = rt.RenderTargetWriteMask;

                    colorBlendAttachments.push_back(attachmentState);
                }

                // 创建颜色混合状态
                VkPipelineColorBlendStateCreateInfo colorBlending = {};
                colorBlending.sType =
                    VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
                colorBlending.logicOpEnable = VK_FALSE;
                colorBlending.attachmentCount =
                    static_cast<uint32_t>(colorBlendAttachments.size());
                colorBlending.pAttachments = colorBlendAttachments.data();

                // 应用混合状态到当前管线
                vulkanCmdList->SetBlendState(colorBlending);
            }

            virtual void SetRasterizerState(
                const RasterizerDesc& desc) override {
                CurrentRasterizerDesc = desc;

                if (!CurrentCommandList) return;

                auto* vulkanCmdList =
                    static_cast<VulkanCommandList*>(CurrentCommandList.get());

                // 创建光栅化状态
                VkPipelineRasterizationStateCreateInfo rasterizer = {};
                rasterizer.sType =
                    VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
                rasterizer.depthClampEnable = VK_FALSE;
                rasterizer.rasterizerDiscardEnable = VK_FALSE;
                rasterizer.polygonMode = desc.FillMode == EFillMode::Wireframe
                                             ? VK_POLYGON_MODE_LINE
                                             : VK_POLYGON_MODE_FILL;

                switch (desc.CullMode) {
                    case ECullMode::None:
                        rasterizer.cullMode = VK_CULL_MODE_NONE;
                        break;
                    case ECullMode::Front:
                        rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
                        break;
                    case ECullMode::Back:
                        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
                        break;
                }

                rasterizer.frontFace = desc.FrontCounterClockwise
                                           ? VK_FRONT_FACE_COUNTER_CLOCKWISE
                                           : VK_FRONT_FACE_CLOCKWISE;
                rasterizer.depthBiasEnable = desc.DepthBias != 0;
                rasterizer.depthBiasConstantFactor =
                    static_cast<float>(desc.DepthBias);
                rasterizer.depthBiasClamp = desc.DepthBiasClamp;
                rasterizer.depthBiasSlopeFactor = desc.SlopeScaledDepthBias;
                rasterizer.lineWidth = 1.0f;

                // 创建多重采样状态
                VkPipelineMultisampleStateCreateInfo multisampling = {};
                multisampling.sType =
                    VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
                multisampling.sampleShadingEnable = desc.MultisampleEnable;
                multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
                multisampling.minSampleShading = 1.0f;
                multisampling.pSampleMask = nullptr;
                multisampling.alphaToCoverageEnable = VK_FALSE;
                multisampling.alphaToOneEnable = VK_FALSE;

                vulkanCmdList->SetRasterizerState(rasterizer, multisampling);
            }

            virtual void SetDepthStencilState(
                const DepthStencilDesc& desc) override {
                CurrentDepthStencilDesc = desc;

                if (!CurrentCommandList) return;

                auto* vulkanCmdList =
                    static_cast<VulkanCommandList*>(CurrentCommandList.get());

                VkPipelineDepthStencilStateCreateInfo depthStencil = {};
                depthStencil.sType =
                    VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
                depthStencil.depthTestEnable = desc.DepthEnable;
                depthStencil.depthWriteEnable = desc.DepthWriteMask;

                // 设置深度比较函数
                switch (desc.DepthFunc) {
                    case ECompareFunction::Never:
                        depthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
                        break;
                    case ECompareFunction::Less:
                        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
                        break;
                    case ECompareFunction::Equal:
                        depthStencil.depthCompareOp = VK_COMPARE_OP_EQUAL;
                        break;
                    case ECompareFunction::LessEqual:
                        depthStencil.depthCompareOp =
                            VK_COMPARE_OP_LESS_OR_EQUAL;
                        break;
                    case ECompareFunction::Greater:
                        depthStencil.depthCompareOp = VK_COMPARE_OP_GREATER;
                        break;
                    case ECompareFunction::NotEqual:
                        depthStencil.depthCompareOp = VK_COMPARE_OP_NOT_EQUAL;
                        break;
                    case ECompareFunction::GreaterEqual:
                        depthStencil.depthCompareOp =
                            VK_COMPARE_OP_GREATER_OR_EQUAL;
                        break;
                    case ECompareFunction::Always:
                        depthStencil.depthCompareOp = VK_COMPARE_OP_ALWAYS;
                        break;
                }

                depthStencil.stencilTestEnable = desc.StencilEnable;

                // 配置前面和背面的模板状态
                auto ConfigureStencilOp = [](VkStencilOpState& state,
                                             const DepthStencilOpDesc& desc) {
                    state.failOp = ConvertToVkStencilOp(desc.StencilFailOp);
                    state.passOp = ConvertToVkStencilOp(desc.StencilPassOp);
                    state.depthFailOp =
                        ConvertToVkStencilOp(desc.StencilDepthFailOp);

                    switch (desc.StencilFunc) {
                        case ECompareFunction::Never:
                            state.compareOp = VK_COMPARE_OP_NEVER;
                            break;
                        case ECompareFunction::Less:
                            state.compareOp = VK_COMPARE_OP_LESS;
                            break;
                        case ECompareFunction::Equal:
                            state.compareOp = VK_COMPARE_OP_EQUAL;
                            break;
                        case ECompareFunction::LessEqual:
                            state.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
                            break;
                        case ECompareFunction::Greater:
                            state.compareOp = VK_COMPARE_OP_GREATER;
                            break;
                        case ECompareFunction::NotEqual:
                            state.compareOp = VK_COMPARE_OP_NOT_EQUAL;
                            break;
                        case ECompareFunction::GreaterEqual:
                            state.compareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
                            break;
                        case ECompareFunction::Always:
                            state.compareOp = VK_COMPARE_OP_ALWAYS;
                            break;
                    }
                };

                ConfigureStencilOp(depthStencil.front, desc.FrontFace);
                ConfigureStencilOp(depthStencil.back, desc.BackFace);

                depthStencil.front.compareMask = desc.StencilReadMask;
                depthStencil.front.writeMask = desc.StencilWriteMask;
                depthStencil.front.reference = desc.StencilRef;
                depthStencil.back.compareMask = desc.StencilReadMask;
                depthStencil.back.writeMask = desc.StencilWriteMask;
                depthStencil.back.reference = desc.StencilRef;

                vulkanCmdList->SetDepthStencilState(depthStencil);
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
                if (!CurrentCommandList) return;

                auto* vulkanCmdList =
                    static_cast<VulkanCommandList*>(CurrentCommandList.get());

                // 应用渲染目标状态
                if (CurrentRenderTarget.RenderTarget ||
                    CurrentRenderTarget.DepthStencil) {
                    vulkanCmdList->SetRenderTargets(
                        1,
                        &CurrentRenderTarget.RenderTarget,
                        CurrentRenderTarget.DepthStencil);
                }

                // 应用视口状态
                if (CurrentViewport.Width > 0 && CurrentViewport.Height > 0) {
                    vulkanCmdList->SetViewport(CurrentViewport);
                }

                // 应用裁剪矩形状态
                if (CurrentScissor.Right > CurrentScissor.Left &&
                    CurrentScissor.Bottom > CurrentScissor.Top) {
                    vulkanCmdList->SetScissorRect(CurrentScissor);
                }

                // 应用混合状态
                VkPipelineColorBlendStateCreateInfo colorBlending = {};
                colorBlending.sType =
                    VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
                colorBlending.logicOpEnable = VK_FALSE;

                std::vector<VkPipelineColorBlendAttachmentState>
                    colorBlendAttachments;
                for (uint32_t i = 0; i < 8; ++i) {
                    const auto& rt = CurrentBlendDesc.RenderTarget[i];
                    VkPipelineColorBlendAttachmentState attachmentState = {};

                    attachmentState.blendEnable = rt.BlendEnable;
                    attachmentState.srcColorBlendFactor =
                        ConvertToVkBlendFactor(rt.SrcBlend);
                    attachmentState.dstColorBlendFactor =
                        ConvertToVkBlendFactor(rt.DestBlend);
                    attachmentState.colorBlendOp =
                        ConvertToVkBlendOp(rt.BlendOp);
                    attachmentState.srcAlphaBlendFactor =
                        ConvertToVkBlendFactor(rt.SrcBlendAlpha);
                    attachmentState.dstAlphaBlendFactor =
                        ConvertToVkBlendFactor(rt.DestBlendAlpha);
                    attachmentState.alphaBlendOp =
                        ConvertToVkBlendOp(rt.BlendOpAlpha);
                    attachmentState.colorWriteMask = rt.RenderTargetWriteMask;

                    colorBlendAttachments.push_back(attachmentState);
                }

                colorBlending.attachmentCount =
                    static_cast<uint32_t>(colorBlendAttachments.size());
                colorBlending.pAttachments = colorBlendAttachments.data();
                vulkanCmdList->SetBlendState(colorBlending);

                // 应用光栅化状态
                VkPipelineRasterizationStateCreateInfo rasterizer = {};
                rasterizer.sType =
                    VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
                rasterizer.depthClampEnable = VK_FALSE;
                rasterizer.rasterizerDiscardEnable = VK_FALSE;
                rasterizer.polygonMode =
                    CurrentRasterizerDesc.FillMode == EFillMode::Wireframe
                        ? VK_POLYGON_MODE_LINE
                        : VK_POLYGON_MODE_FILL;

                switch (CurrentRasterizerDesc.CullMode) {
                    case ECullMode::None:
                        rasterizer.cullMode = VK_CULL_MODE_NONE;
                        break;
                    case ECullMode::Front:
                        rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
                        break;
                    case ECullMode::Back:
                        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
                        break;
                }

                rasterizer.frontFace =
                    CurrentRasterizerDesc.FrontCounterClockwise
                        ? VK_FRONT_FACE_COUNTER_CLOCKWISE
                        : VK_FRONT_FACE_CLOCKWISE;
                rasterizer.depthBiasEnable =
                    CurrentRasterizerDesc.DepthBias != 0;
                rasterizer.depthBiasConstantFactor =
                    static_cast<float>(CurrentRasterizerDesc.DepthBias);
                rasterizer.depthBiasClamp =
                    CurrentRasterizerDesc.DepthBiasClamp;
                rasterizer.depthBiasSlopeFactor =
                    CurrentRasterizerDesc.SlopeScaledDepthBias;
                rasterizer.lineWidth = 1.0f;

                // 创建多重采样状态
                VkPipelineMultisampleStateCreateInfo multisampling = {};
                multisampling.sType =
                    VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
                multisampling.sampleShadingEnable =
                    CurrentRasterizerDesc.MultisampleEnable;
                multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
                multisampling.minSampleShading = 1.0f;
                multisampling.pSampleMask = nullptr;
                multisampling.alphaToCoverageEnable = VK_FALSE;
                multisampling.alphaToOneEnable = VK_FALSE;

                vulkanCmdList->SetRasterizerState(rasterizer, multisampling);

                // 应用深度模板状态
                VkPipelineDepthStencilStateCreateInfo depthStencil = {};
                depthStencil.sType =
                    VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
                depthStencil.depthTestEnable =
                    CurrentDepthStencilDesc.DepthEnable;
                depthStencil.depthWriteEnable =
                    CurrentDepthStencilDesc.DepthWriteMask;

                switch (CurrentDepthStencilDesc.DepthFunc) {
                    case ECompareFunction::Never:
                        depthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
                        break;
                    case ECompareFunction::Less:
                        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
                        break;
                    case ECompareFunction::Equal:
                        depthStencil.depthCompareOp = VK_COMPARE_OP_EQUAL;
                        break;
                    case ECompareFunction::LessEqual:
                        depthStencil.depthCompareOp =
                            VK_COMPARE_OP_LESS_OR_EQUAL;
                        break;
                    case ECompareFunction::Greater:
                        depthStencil.depthCompareOp = VK_COMPARE_OP_GREATER;
                        break;
                    case ECompareFunction::NotEqual:
                        depthStencil.depthCompareOp = VK_COMPARE_OP_NOT_EQUAL;
                        break;
                    case ECompareFunction::GreaterEqual:
                        depthStencil.depthCompareOp =
                            VK_COMPARE_OP_GREATER_OR_EQUAL;
                        break;
                    case ECompareFunction::Always:
                        depthStencil.depthCompareOp = VK_COMPARE_OP_ALWAYS;
                        break;
                }

                depthStencil.stencilTestEnable =
                    CurrentDepthStencilDesc.StencilEnable;

                // 配置前面和背面的模板状态
                auto ConfigureStencilOp = [](VkStencilOpState& state,
                                             const DepthStencilOpDesc& desc) {
                    state.failOp = ConvertToVkStencilOp(desc.StencilFailOp);
                    state.passOp = ConvertToVkStencilOp(desc.StencilPassOp);
                    state.depthFailOp =
                        ConvertToVkStencilOp(desc.StencilDepthFailOp);

                    switch (desc.StencilFunc) {
                        case ECompareFunction::Never:
                            state.compareOp = VK_COMPARE_OP_NEVER;
                            break;
                        case ECompareFunction::Less:
                            state.compareOp = VK_COMPARE_OP_LESS;
                            break;
                        case ECompareFunction::Equal:
                            state.compareOp = VK_COMPARE_OP_EQUAL;
                            break;
                        case ECompareFunction::LessEqual:
                            state.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
                            break;
                        case ECompareFunction::Greater:
                            state.compareOp = VK_COMPARE_OP_GREATER;
                            break;
                        case ECompareFunction::NotEqual:
                            state.compareOp = VK_COMPARE_OP_NOT_EQUAL;
                            break;
                        case ECompareFunction::GreaterEqual:
                            state.compareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
                            break;
                        case ECompareFunction::Always:
                            state.compareOp = VK_COMPARE_OP_ALWAYS;
                            break;
                    }
                };

                ConfigureStencilOp(depthStencil.front,
                                   CurrentDepthStencilDesc.FrontFace);
                ConfigureStencilOp(depthStencil.back,
                                   CurrentDepthStencilDesc.BackFace);

                depthStencil.front.compareMask =
                    CurrentDepthStencilDesc.StencilReadMask;
                depthStencil.front.writeMask =
                    CurrentDepthStencilDesc.StencilWriteMask;
                depthStencil.back.compareMask =
                    CurrentDepthStencilDesc.StencilReadMask;
                depthStencil.back.writeMask =
                    CurrentDepthStencilDesc.StencilWriteMask;

                vulkanCmdList->SetDepthStencilState(depthStencil);
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

            RHIDevicePtr Device;
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
