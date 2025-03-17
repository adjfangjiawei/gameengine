
#pragma once

#include <vulkan/vulkan.h>

#include <vector>

#include "RHI/RHIModule.h"
#include "VulkanPhysicalDevice.h"

namespace Engine {
    namespace RHI {

        class VulkanDevice {
          public:
            ~VulkanDevice();

            bool Initialize(VulkanPhysicalDevice* physicalDevice);
            void Shutdown();

            // 设备相关
            const VkDevice* GetHandle() const { return &Device; }
            VulkanPhysicalDevice* GetPhysicalDevice() const {
                return PhysicalDevice;
            }

            // 队列相关
            uint32_t GetGraphicsQueueFamilyIndex() const;
            uint32_t GetComputeQueueFamilyIndex() const;
            uint32_t GetTransferQueueFamilyIndex() const;
            const VkQueue* GetGraphicsQueue() const { return &GraphicsQueue; }
            const VkQueue* GetComputeQueue() const { return &ComputeQueue; }
            const VkQueue* GetTransferQueue() const { return &TransferQueue; }

            // 格式转换
            static VkFormat ConvertToVkFormat(EPixelFormat format);

            // 资源创建
            IRHIBuffer* CreateBuffer(const BufferDesc& desc);
            IRHITexture* CreateTexture(const TextureDesc& desc);
            IRHIShader* CreateShader(const ShaderDesc& desc,
                                     const void* shaderData,
                                     size_t dataSize);
            IRHIRenderTargetView* CreateRenderTargetView(
                IRHIResource* resource, const RenderTargetViewDesc& desc);
            IRHIDepthStencilView* CreateDepthStencilView(
                IRHIResource* resource, const DepthStencilViewDesc& desc);
            IRHIShaderResourceView* CreateShaderResourceView(
                IRHIResource* resource, const ShaderResourceViewDesc& desc);
            IRHIUnorderedAccessView* CreateUnorderedAccessView(
                IRHIResource* resource, const UnorderedAccessViewDesc& desc);
            IRHISamplerState* CreateSamplerState(const SamplerDesc& desc);
            IRHISwapChain* CreateSwapChain(const SwapChainDesc& desc);
            IRHIEvent* CreateEvent();
            IRHITimelineSemaphore* CreateTimelineSemaphore(
                const TimelineSemaphoreDesc& desc);
            IRHIShaderFeedbackBuffer* CreateShaderFeedbackBuffer(
                const ShaderFeedbackDesc& desc);
            IRHIPredicate* CreatePredicate(const PredicateDesc& desc);

            // 命令提交
            void SubmitCommandLists(uint32 count,
                                    IRHICommandList* const* commandLists);
            void WaitForGPU();
            void SignalGPU();

            // 特性支持
            ERHIFeatureLevel GetFeatureLevel() const;
            bool SupportsRayTracing() const;
            bool SupportsVariableRateShading() const;
            bool SupportsMeshShaders() const;

            // 内存管理
            uint64 GetTotalVideoMemory() const;
            uint64 GetAvailableVideoMemory() const;
            void GetStats(RHIStats& stats) const;

            // 帧管理
            void BeginFrame();
            void EndFrame();

            // 调试
            void SetDebugName(IRHIResource* resource, const char* name);
            void SetDebugName(VkObjectType objectType,
                              uint64_t object,
                              const char* name);

          private:
            bool CreateLogicalDevice();
            std::vector<const char*> GetRequiredDeviceExtensions() const;
            bool IsDepthFormat(EPixelFormat format) const;
            bool IsStencilFormat(EPixelFormat format) const;

          private:
            VulkanPhysicalDevice* PhysicalDevice = nullptr;
            VkDevice Device = VK_NULL_HANDLE;
            VkQueue GraphicsQueue = VK_NULL_HANDLE;
            VkQueue ComputeQueue = VK_NULL_HANDLE;
            VkQueue TransferQueue = VK_NULL_HANDLE;
        };

    }  // namespace RHI
}  // namespace Engine
