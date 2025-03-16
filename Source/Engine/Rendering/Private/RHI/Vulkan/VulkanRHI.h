
#pragma once

#include <vulkan/vulkan.h>

#include <memory>
#include <unordered_map>
#include <vector>

#include "RHI/RHI.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIDefinitions.h"
#include "RHI/RHIDevice.h"
#include "RHI/RHIResources.h"
#include "VulkanMemory.h"

namespace Engine {
    namespace RHI {

        // Vulkan实例包装器
        class VulkanInstance {
          public:
            VulkanInstance() = default;
            ~VulkanInstance();

            bool Initialize(const RHIModuleInitParams& params);
            void Shutdown();

            VkInstance GetHandle() const { return Instance; }
            bool SupportsValidationLayers() const {
                return ValidationLayersEnabled;
            }

          private:
            bool CreateInstance(const RHIModuleInitParams& params);
            bool SetupDebugMessenger();
            std::vector<const char*> GetRequiredExtensions() const;
            bool CheckValidationLayerSupport() const;

            VkInstance Instance = VK_NULL_HANDLE;
            VkDebugUtilsMessengerEXT DebugMessenger = VK_NULL_HANDLE;
            bool ValidationLayersEnabled = false;

            static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
                VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                void* pUserData);
        };

        // Vulkan物理设备包装器
        class VulkanPhysicalDevice {
          public:
            struct QueueFamilyIndices {
                uint32_t GraphicsFamily = UINT32_MAX;
                uint32_t ComputeFamily = UINT32_MAX;
                uint32_t TransferFamily = UINT32_MAX;
                uint32_t PresentFamily = UINT32_MAX;

                bool IsComplete() const {
                    return GraphicsFamily != UINT32_MAX &&
                           ComputeFamily != UINT32_MAX &&
                           TransferFamily != UINT32_MAX;
                }
            };

            VulkanPhysicalDevice() = default;
            ~VulkanPhysicalDevice() = default;

            bool Initialize(VkInstance instance);

            VkPhysicalDevice GetHandle() const { return PhysicalDevice; }
            VkInstance GetInstance() const { return Instance; }
            const VkPhysicalDeviceProperties& GetProperties() const {
                return Properties;
            }
            const VkPhysicalDeviceFeatures& GetFeatures() const {
                return Features;
            }
            const QueueFamilyIndices& GetQueueFamilyIndices() const {
                return QueueFamilies;
            }
            ERHIFeatureLevel GetFeatureLevel() const { return FeatureLevel; }
            VulkanMemoryAllocator* GetMemoryAllocator() const {
                return MemoryAllocator.get();
            }
            void SetMemoryAllocator(
                std::unique_ptr<VulkanMemoryAllocator> allocator) {
                MemoryAllocator = std::move(allocator);
            }

          private:
            std::unique_ptr<VulkanMemoryAllocator> MemoryAllocator;
            bool SelectPhysicalDevice(VkInstance instance);
            int RateDeviceSuitability(VkPhysicalDevice device) const;
            QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;
            ERHIFeatureLevel DetermineFeatureLevel(
                const VkPhysicalDeviceProperties& props) const;

            VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
            VkPhysicalDeviceProperties Properties = {};
            VkPhysicalDeviceFeatures Features = {};
            QueueFamilyIndices QueueFamilies;
            ERHIFeatureLevel FeatureLevel = ERHIFeatureLevel::ES2;
            VkInstance Instance = VK_NULL_HANDLE;
        };

        // Vulkan逻辑设备包装器，实现IRHIDevice接口
        class VulkanDevice : public IRHIDevice {
          public:
            VulkanDevice() = default;
            ~VulkanDevice();

            bool Initialize(VulkanPhysicalDevice* physicalDevice);
            void Shutdown();

            // IRHIDevice接口实现
            virtual IRHIBuffer* CreateBuffer(const BufferDesc& desc) override;
            virtual IRHITexture* CreateTexture(
                const TextureDesc& desc) override;
            virtual IRHIShader* CreateShader(const ShaderDesc& desc,
                                             const void* shaderData,
                                             size_t dataSize) override;
            virtual IRHIRenderTargetView* CreateRenderTargetView(
                IRHIResource* resource,
                const RenderTargetViewDesc& desc) override;
            virtual IRHIDepthStencilView* CreateDepthStencilView(
                IRHIResource* resource,
                const DepthStencilViewDesc& desc) override;

            // 辅助函数
            bool IsDepthFormat(EPixelFormat format) const;
            bool IsStencilFormat(EPixelFormat format) const;
            static VkFormat ConvertToVkFormat(EPixelFormat format);
            virtual IRHIShaderResourceView* CreateShaderResourceView(
                IRHIResource* resource,
                const ShaderResourceViewDesc& desc) override;
            virtual IRHIUnorderedAccessView* CreateUnorderedAccessView(
                IRHIResource* resource,
                const UnorderedAccessViewDesc& desc) override;
            virtual IRHISamplerState* CreateSamplerState(
                const SamplerDesc& desc) override;
            virtual RHICommandAllocatorPtr CreateCommandAllocator(
                ECommandListType /*type*/) override {
                // TODO: Implement actual Vulkan command pool creation
                return nullptr;
            }

            virtual RHICommandListPtr CreateCommandList(
                ECommandListType /*type*/,
                IRHICommandAllocator* /*allocator*/) override {
                // TODO: Implement actual Vulkan command buffer creation
                return nullptr;
            }
            virtual IRHISwapChain* CreateSwapChain(
                const SwapChainDesc& desc) override;
            virtual void SubmitCommandLists(
                uint32 count, IRHICommandList* const* commandLists) override;
            virtual void WaitForGPU() override;
            virtual void SignalGPU() override;
            virtual ERHIFeatureLevel GetFeatureLevel() const override;
            virtual bool SupportsRayTracing() const override;
            virtual bool SupportsVariableRateShading() const override;
            virtual bool SupportsMeshShaders() const override;
            virtual uint64 GetTotalVideoMemory() const override;
            virtual uint64 GetAvailableVideoMemory() const override;
            virtual void GetStats(RHIStats& stats) const override;
            virtual void BeginFrame() override;
            virtual void EndFrame() override;
            virtual void SetDebugName(IRHIResource* resource,
                                      const char* name) override;

            // Vulkan特定接口
            VkDevice GetHandle() const { return Device; }
            VulkanPhysicalDevice* GetPhysicalDevice() const {
                return PhysicalDevice;
            }
            VkQueue GetGraphicsQueue() const { return GraphicsQueue; }
            VkQueue GetComputeQueue() const { return ComputeQueue; }
            VkQueue GetTransferQueue() const { return TransferQueue; }
            uint32_t GetGraphicsQueueFamilyIndex() const;
            uint32_t GetComputeQueueFamilyIndex() const;
            uint32_t GetTransferQueueFamilyIndex() const;

          private:
            bool CreateLogicalDevice();
            std::vector<const char*> GetRequiredDeviceExtensions() const;

            VkDevice Device = VK_NULL_HANDLE;
            VulkanPhysicalDevice* PhysicalDevice = nullptr;
            VkQueue GraphicsQueue = VK_NULL_HANDLE;
            VkQueue ComputeQueue = VK_NULL_HANDLE;
            VkQueue TransferQueue = VK_NULL_HANDLE;
        };

        // Vulkan命令池管理器
        class VulkanCommandPoolManager {
          public:
            VulkanCommandPoolManager() = default;
            ~VulkanCommandPoolManager();

            bool Initialize(VulkanDevice* device);
            void Shutdown();

            VkCommandPool GetGraphicsCommandPool() const {
                return GraphicsCommandPool;
            }
            VkCommandPool GetComputeCommandPool() const {
                return ComputeCommandPool;
            }
            VkCommandPool GetTransferCommandPool() const {
                return TransferCommandPool;
            }

          private:
            bool CreateCommandPools();

            VulkanDevice* Device = nullptr;
            VkCommandPool GraphicsCommandPool = VK_NULL_HANDLE;
            VkCommandPool ComputeCommandPool = VK_NULL_HANDLE;
            VkCommandPool TransferCommandPool = VK_NULL_HANDLE;
        };

        // 前向声明
        class VulkanMemoryAllocator;

        // Vulkan RHI实现类，继承自IRHIModule
        class VulkanRHI : public IRHIModule {
          public:
            static VulkanRHI& Get();

            // IRHIModule接口实现
            virtual bool Initialize(const RHIModuleInitParams& params) override;
            virtual void Shutdown() override;
            virtual IRHIDevice* GetDevice() override { return &Device; }
            virtual IRHIContext* GetImmediateContext() override {
                return ImmediateContext.get();
            }
            virtual bool IsDeviceLost() const override { return false; }
            virtual bool ResetDevice() override { return true; }
            virtual ERHIModuleState GetState() const override { return State; }
            virtual ERHIFeatureLevel GetMaxFeatureLevel() const override {
                return PhysicalDevice.GetFeatureLevel();
            }
            virtual const RHIStats& GetStats() const override {
                return Statistics;
            }
            virtual void EnableDebugLayer(bool enable) override;
            virtual void EnableGPUValidation(bool enable) override;
            virtual void EnableProfiling(bool enable) override;
            virtual void CaptureNextFrame() override;
            virtual void BeginCapture() override;
            virtual void EndCapture() override;
            virtual void BeginFrame() override;
            virtual void EndFrame() override;
            virtual uint64 GetFrameCount() const override { return FrameCount; }
            virtual void ReleaseUnusedResources() override;
            virtual uint64 GetTotalResourceMemory() const override;
            virtual uint64 GetUsedResourceMemory() const override;
            virtual void SetDeviceLostCallback(
                std::function<void()> callback) override;
            virtual void SetDeviceRestoredCallback(
                std::function<void()> callback) override;
            virtual void SetErrorCallback(
                std::function<void(const char*)> callback) override;

          protected:
            virtual void OnDeviceLost() override;
            virtual void OnDeviceRestored() override;
            virtual void OnError(const char* message) override;

          private:
            VulkanRHI() = default;
            ~VulkanRHI() = default;

            VulkanRHI(const VulkanRHI&) = delete;
            VulkanRHI& operator=(const VulkanRHI&) = delete;

            VulkanInstance Instance;
            VulkanPhysicalDevice PhysicalDevice;
            VulkanDevice Device;
            VulkanCommandPoolManager CommandPoolManager;
            std::unique_ptr<VulkanMemoryAllocator> MemoryAllocator;
            std::unique_ptr<IRHIContext> ImmediateContext;

            ERHIModuleState State = ERHIModuleState::Uninitialized;
            RHIStats Statistics = {};
            uint64 FrameCount = 0;

            std::function<void()> DeviceLostCallback;
            std::function<void()> DeviceRestoredCallback;
            std::function<void(const char*)> ErrorCallback;

            bool DebugLayerEnabled = false;
            bool GPUValidationEnabled = false;
            bool ProfilingEnabled = false;
        };

    }  // namespace RHI
}  // namespace Engine
