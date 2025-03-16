
#include <array>

#include "Core/Public/Log/LogSystem.h"
#include "RHI/RHI.h"

namespace Engine {
    namespace RHI {

        class RHIExample {
          public:
            bool Initialize(void* windowHandle,
                            uint32_t width,
                            uint32_t height) {
                // 初始化RHI
                RHIModuleInitParams initParams;
                initParams.DeviceParams.EnableDebugLayer = true;
                initParams.DeviceParams.ApplicationName = "RHI Example";
                initParams.DeviceParams.ApplicationVersion = 1;
                initParams.EnableValidation = true;

                if (!Engine::RHI::Initialize(initParams)) {
                    LOG_ERROR("Failed to initialize RHI!");
                    return false;
                }

                // 获取设备和上下文
                Device = GetDevice();
                Context = GetImmediateContext();

                if (!Device || !Context) {
                    LOG_ERROR("Failed to get device or context!");
                    return false;
                }

                // 创建交换链
                SwapChainDesc swapChainDesc;
                swapChainDesc.WindowHandle = windowHandle;
                swapChainDesc.Width = width;
                swapChainDesc.Height = height;
                swapChainDesc.BufferCount = 2;
                swapChainDesc.Format = EPixelFormat::R8G8B8A8_UNORM;
                swapChainDesc.VSync = true;

                SwapChain = Device->CreateSwapChain(swapChainDesc);
                if (!SwapChain) {
                    LOG_ERROR("Failed to create swap chain!");
                    return false;
                }

                // 创建渲染目标视图
                for (uint32_t i = 0; i < swapChainDesc.BufferCount; ++i) {
                    RenderTargetViewDesc rtvDesc;
                    rtvDesc.Format = swapChainDesc.Format;
                    auto rtv = Device->CreateRenderTargetView(
                        SwapChain->GetBackBuffer(i), rtvDesc);
                    if (!rtv) {
                        LOG_ERROR("Failed to create render target view!");
                        return false;
                    }
                    RenderTargetViews.push_back(rtv);
                }

                // 创建深度缓冲区
                TextureDesc depthDesc;
                depthDesc.Width = width;
                depthDesc.Height = height;
                depthDesc.Format = EPixelFormat::D24_UNORM_S8_UINT;
                depthDesc.Flags = ERHIResourceFlags::AllowDepthStencil;

                DepthTexture = Device->CreateTexture(depthDesc);
                if (!DepthTexture) {
                    LOG_ERROR("Failed to create depth texture!");
                    return false;
                }

                // 创建深度模板视图
                DepthStencilViewDesc dsvDesc;
                dsvDesc.Format = depthDesc.Format;
                DepthStencilView =
                    Device->CreateDepthStencilView(DepthTexture, dsvDesc);
                if (!DepthStencilView) {
                    LOG_ERROR("Failed to create depth stencil view!");
                    return false;
                }

                // 创建顶点缓冲区
                struct Vertex {
                    float position[3];
                    float color[4];
                };

                std::array<Vertex, 3> vertices = {
                    {{{0.0f, 0.5f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
                     {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
                     {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}}}};

                BufferDesc vbDesc;
                vbDesc.SizeInBytes = sizeof(vertices);
                vbDesc.Stride = sizeof(Vertex);
                vbDesc.Flags = ERHIResourceFlags::None;
                vbDesc.Access =
                    ERHIAccessFlags::CPUWrite | ERHIAccessFlags::GPURead;

                VertexBuffer = Device->CreateBuffer(vbDesc);
                if (!VertexBuffer) {
                    LOG_ERROR("Failed to create vertex buffer!");
                    return false;
                }

                // 上传顶点数据
                void* data = VertexBuffer->Map();
                memcpy(data, vertices.data(), sizeof(vertices));
                VertexBuffer->Unmap();

                // 创建着色器
                const char* vertexShaderCode = R"(
            #version 450
            layout(location = 0) in vec3 inPosition;
            layout(location = 1) in vec4 inColor;
            layout(location = 0) out vec4 outColor;
            void main() {
                gl_Position = vec4(inPosition, 1.0);
                outColor = inColor;
            }
        )";

                const char* pixelShaderCode = R"(
            #version 450
            layout(location = 0) in vec4 inColor;
            layout(location = 0) out vec4 outColor;
            void main() {
                outColor = inColor;
            }
        )";

                ShaderDesc vsDesc;
                vsDesc.Type = EShaderType::Vertex;
                vsDesc.EntryPoint = "main";
                VertexShader = Device->CreateShader(
                    vsDesc, vertexShaderCode, strlen(vertexShaderCode));

                ShaderDesc psDesc;
                psDesc.Type = EShaderType::Pixel;
                psDesc.EntryPoint = "main";
                PixelShader = Device->CreateShader(
                    psDesc, pixelShaderCode, strlen(pixelShaderCode));

                if (!VertexShader || !PixelShader) {
                    LOG_ERROR("Failed to create shaders!");
                    return false;
                }

                // 创建命令分配器
                CommandAllocator =
                    Device->CreateCommandAllocator(ECommandListType::Direct);
                if (!CommandAllocator) {
                    LOG_ERROR("Failed to create command allocator!");
                    return false;
                }

                return true;
            }

            void Cleanup() {
                if (Device) {
                    Device->WaitForGPU();
                }

                for (auto rtv : RenderTargetViews) {
                    delete rtv;
                }
                RenderTargetViews.clear();

                delete DepthStencilView;
                delete DepthTexture;
                delete VertexBuffer;
                delete VertexShader;
                delete PixelShader;
                delete SwapChain;

                CommandAllocator.reset();

                Shutdown();
            }

            void Render() {
                BeginFrame();

                // 获取当前后备缓冲区
                uint32_t backBufferIndex =
                    SwapChain->GetCurrentBackBufferIndex();
                auto rtv = RenderTargetViews[backBufferIndex];

                // 设置渲染目标
                RenderTargetBinding rtBinding;
                rtBinding.RenderTarget = rtv;
                rtBinding.DepthStencil = DepthStencilView;
                Context->SetRenderTargets(rtBinding);

                // 清除渲染目标
                float clearColor[] = {0.0f, 0.2f, 0.4f, 1.0f};
                Context->ClearRenderTarget(rtv, clearColor);
                Context->ClearDepthStencil(DepthStencilView, 1.0f, 0);

                // 设置视口和裁剪矩形
                Viewport viewport;
                viewport.Width = static_cast<float>(SwapChain->GetDesc().Width);
                viewport.Height =
                    static_cast<float>(SwapChain->GetDesc().Height);
                viewport.MinDepth = 0.0f;
                viewport.MaxDepth = 1.0f;
                Context->SetViewport(viewport);

                ScissorRect scissor;
                scissor.Right = SwapChain->GetDesc().Width;
                scissor.Bottom = SwapChain->GetDesc().Height;
                Context->SetScissorRect(scissor);

                // 设置着色器
                ShaderBinding vsBinding;
                vsBinding.Shader = VertexShader;
                Context->SetVertexShader(vsBinding);

                ShaderBinding psBinding;
                psBinding.Shader = PixelShader;
                Context->SetPixelShader(psBinding);

                // 设置顶点缓冲区
                VertexStreamBinding vertexBinding;
                VertexBufferView vbView;
                vbView.Buffer = VertexBuffer;
                vbView.Offset = 0;
                vbView.Stride = sizeof(float) * 7;  // position(3) + color(4)
                vertexBinding.VertexBuffers.push_back(vbView);
                vertexBinding.PrimitiveType = EPrimitiveType::Triangles;
                Context->SetVertexBuffers(vertexBinding);

                // 绘制三角形
                Context->Draw(3, 0);

                EndFrame();

                // 呈现
                SwapChain->Present();
            }

          private:
            IRHIDevice* Device = nullptr;
            IRHIContext* Context = nullptr;
            IRHISwapChain* SwapChain = nullptr;
            std::vector<IRHIRenderTargetView*> RenderTargetViews;
            IRHITexture* DepthTexture = nullptr;
            IRHIDepthStencilView* DepthStencilView = nullptr;
            IRHIBuffer* VertexBuffer = nullptr;
            IRHIShader* VertexShader = nullptr;
            IRHIShader* PixelShader = nullptr;
            RHICommandAllocatorPtr CommandAllocator;
        };

    }  // namespace RHI
}  // namespace Engine
