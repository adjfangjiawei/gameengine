
#pragma once

#include <memory>

namespace Engine {
    namespace RHI {
        class IRHIDevice;
        class IRHIContext;
        class IRHISwapChain;
        class IRHITexture;
        class IRHIBuffer;
        class IRHIShader;
        class IRHIRenderTargetView;
        class IRHIDepthStencilView;
        class IRHICommandAllocator;

        using RHIDevicePtr = std::shared_ptr<IRHIDevice>;
        using RHIContextPtr = std::shared_ptr<IRHIContext>;
        using RHISwapChainPtr = std::shared_ptr<IRHISwapChain>;
        using RHITexturePtr = std::shared_ptr<IRHITexture>;
        using RHIBufferPtr = std::shared_ptr<IRHIBuffer>;
        using RHIShaderPtr = std::shared_ptr<IRHIShader>;
        using RHIRenderTargetViewPtr = std::shared_ptr<IRHIRenderTargetView>;
        using RHIDepthStencilViewPtr = std::shared_ptr<IRHIDepthStencilView>;
        using RHICommandAllocatorPtr = std::shared_ptr<IRHICommandAllocator>;
    }  // namespace RHI
}  // namespace Engine
