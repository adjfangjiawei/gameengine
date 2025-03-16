
#pragma once

#include <float.h>

#include <vector>

#include "RHICommandList.h"

namespace Engine {
    namespace RHI {

        // 设备创建参数
        struct RHIDeviceCreateParams {
            bool EnableDebugLayer = false;
            bool EnableGPUValidation = false;
            ERHIFeatureLevel MinFeatureLevel = ERHIFeatureLevel::ES3_1;
            std::string ApplicationName;
            uint32 ApplicationVersion = 1;
        };

        // 交换链描述
        struct SwapChainDesc {
            void* WindowHandle = nullptr;  // 平台相关的窗口句柄
            void* Connection =
                nullptr;  // 平台相关的连接句柄（Linux下为XCB连接）
            uint32 Width = 0;
            uint32 Height = 0;
            uint32 BufferCount = 2;
            EPixelFormat Format = EPixelFormat::R8G8B8A8_UNORM;
            bool VSync = true;
            std::string DebugName;
        };

        // 交换链接口
        class IRHISwapChain : public IRHIResource {
          public:
            // 获取后备缓冲区
            virtual IRHITexture* GetBackBuffer(uint32 index) = 0;

            // 获取当前后备缓冲区索引
            virtual uint32 GetCurrentBackBufferIndex() const = 0;

            // 呈现
            virtual void Present() = 0;

            // 调整大小
            virtual void Resize(uint32 width, uint32 height) = 0;

            // 获取描述
            virtual const SwapChainDesc& GetDesc() const = 0;
        };

        // 采样器描述
        struct SamplerDesc {
            ESamplerFilter Filter = ESamplerFilter::MinMagMipLinear;
            ESamplerAddressMode AddressU = ESamplerAddressMode::Wrap;
            ESamplerAddressMode AddressV = ESamplerAddressMode::Wrap;
            ESamplerAddressMode AddressW = ESamplerAddressMode::Wrap;
            float MipLODBias = 0.0f;
            uint32 MaxAnisotropy = 1;
            ECompareFunction ComparisonFunc = ECompareFunction::Never;
            float BorderColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
            float MinLOD = 0.0f;
            float MaxLOD = FLT_MAX;
            std::string DebugName;
        };

        // 采样器状态接口
        class IRHISamplerState : public IRHIResource {
          public:
            virtual const SamplerDesc& GetDesc() const = 0;
        };

        // 栅格化状态描述
        struct RasterizerDesc {
            EFillMode FillMode = EFillMode::Solid;
            ECullMode CullMode = ECullMode::Back;
            bool FrontCounterClockwise = false;
            int32 DepthBias = 0;
            float DepthBiasClamp = 0.0f;
            float SlopeScaledDepthBias = 0.0f;
            bool DepthClipEnable = true;
            bool MultisampleEnable = false;
            bool AntialiasedLineEnable = false;
            bool ConservativeRaster = false;
            std::string DebugName;
        };

        // 深度模板状态描述
        struct DepthStencilDesc {
            bool DepthEnable = true;
            bool DepthWriteMask = true;
            ECompareFunction DepthFunc = ECompareFunction::Less;
            bool StencilEnable = false;
            uint8 StencilReadMask = 0xFF;
            uint8 StencilWriteMask = 0xFF;
            std::string DebugName;
        };

        // 混合状态描述
        struct BlendDesc {
            bool AlphaToCoverageEnable = false;
            bool IndependentBlendEnable = false;
            struct RenderTarget {
                bool BlendEnable = false;
                EBlendFactor SrcBlend = EBlendFactor::One;
                EBlendFactor DestBlend = EBlendFactor::Zero;
                EBlendOp BlendOp = EBlendOp::Add;
                EBlendFactor SrcBlendAlpha = EBlendFactor::One;
                EBlendFactor DestBlendAlpha = EBlendFactor::Zero;
                EBlendOp BlendOpAlpha = EBlendOp::Add;
                uint8 RenderTargetWriteMask = 0x0F;
            } RenderTarget[8];
            std::string DebugName;
        };

        // 设备接口
        class IRHIDevice {
          public:
            virtual ~IRHIDevice() = default;

            // 资源创建
            virtual IRHIBuffer* CreateBuffer(const BufferDesc& desc) = 0;
            virtual IRHITexture* CreateTexture(const TextureDesc& desc) = 0;
            virtual IRHIShader* CreateShader(const ShaderDesc& desc,
                                             const void* shaderData,
                                             size_t dataSize) = 0;

            // 视图创建
            virtual IRHIRenderTargetView* CreateRenderTargetView(
                IRHIResource* resource, const RenderTargetViewDesc& desc) = 0;
            virtual IRHIDepthStencilView* CreateDepthStencilView(
                IRHIResource* resource, const DepthStencilViewDesc& desc) = 0;
            virtual IRHIShaderResourceView* CreateShaderResourceView(
                IRHIResource* resource, const ShaderResourceViewDesc& desc) = 0;
            virtual IRHIUnorderedAccessView* CreateUnorderedAccessView(
                IRHIResource* resource,
                const UnorderedAccessViewDesc& desc) = 0;

            // 状态对象创建
            virtual IRHISamplerState* CreateSamplerState(
                const SamplerDesc& desc) = 0;

            // 命令分配器和列表创建
            virtual RHICommandAllocatorPtr CreateCommandAllocator(
                ECommandListType type) = 0;
            virtual RHICommandListPtr CreateCommandList(
                ECommandListType type, IRHICommandAllocator* allocator) = 0;

            // 交换链创建
            virtual IRHISwapChain* CreateSwapChain(
                const SwapChainDesc& desc) = 0;

            // 命令提交和同步
            virtual void SubmitCommandLists(
                uint32 count, IRHICommandList* const* commandLists) = 0;

            virtual void WaitForGPU() = 0;
            virtual void SignalGPU() = 0;

            // 设备查询
            virtual ERHIFeatureLevel GetFeatureLevel() const = 0;
            virtual bool SupportsRayTracing() const = 0;
            virtual bool SupportsVariableRateShading() const = 0;
            virtual bool SupportsMeshShaders() const = 0;

            // 内存管理
            virtual uint64 GetTotalVideoMemory() const = 0;
            virtual uint64 GetAvailableVideoMemory() const = 0;

            // 统计信息
            virtual void GetStats(RHIStats& stats) const = 0;

            // 调试支持
            virtual void BeginFrame() = 0;
            virtual void EndFrame() = 0;
            virtual void SetDebugName(IRHIResource* resource,
                                      const char* name) = 0;
        };

        // 智能指针类型定义
        using RHIDevicePtr = std::shared_ptr<IRHIDevice>;

        // 创建RHI设备
        RHIDevicePtr CreateRHIDevice(const RHIDeviceCreateParams& params);

    }  // namespace RHI
}  // namespace Engine
