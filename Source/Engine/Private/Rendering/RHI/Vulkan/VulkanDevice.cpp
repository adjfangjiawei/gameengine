
#include <algorithm>
#include <set>

#include "Core/Log/LogSystem.h"
#include "VulkanRHI.h"

namespace Engine {
    namespace RHI {

        // VulkanDevice Implementation
        class VulkanDeviceImpl : public IRHIDevice {
          public:
            VulkanDeviceImpl(VulkanRHI* rhi)
                : RHI(rhi), Device(rhi->GetDevice()) {}

            virtual ~VulkanDeviceImpl() = default;

            // 资源创建
            virtual IRHIBuffer* CreateBuffer(const BufferDesc& desc) override {
                return new VulkanBuffer(Device, desc);
            }

            virtual IRHITexture* CreateTexture(
                const TextureDesc& desc) override {
                return new VulkanTexture(Device, desc);
            }

            virtual IRHIShader* CreateShader(const ShaderDesc& desc,
                                             const void* shaderData,
                                             size_t dataSize) override {
                return new VulkanShader(Device, desc, shaderData, dataSize);
            }

            // 视图创建
            virtual IRHIRenderTargetView* CreateRenderTargetView(
                IRHIResource* resource,
                const RenderTargetViewDesc& desc) override {
                return new VulkanRenderTargetView(Device, resource, desc);
            }

            virtual IRHIDepthStencilView* CreateDepthStencilView(
                IRHIResource* resource,
                const DepthStencilViewDesc& desc) override {
                return new VulkanDepthStencilView(Device, resource, desc);
            }

            virtual IRHIShaderResourceView* CreateShaderResourceView(
                IRHIResource* resource,
                const ShaderResourceViewDesc& desc) override {
                return new VulkanShaderResourceView(Device, resource, desc);
            }

            virtual IRHIUnorderedAccessView* CreateUnorderedAccessView(
                IRHIResource* resource,
                const UnorderedAccessViewDesc& desc) override {
                return new VulkanUnorderedAccessView(Device, resource, desc);
            }

            // 状态对象创建
            virtual IRHISamplerState* CreateSamplerState(
                const SamplerDesc& desc) override {
                // TODO: 实现采样器状态创建
                return nullptr;
            }

            // 命令分配器和列表创建
            virtual RHICommandAllocatorPtr CreateCommandAllocator(
                ECommandListType type) override {
                return std::make_shared<VulkanCommandAllocator>(Device, type);
            }

            virtual RHICommandListPtr CreateCommandList(
                ECommandListType type,
                IRHICommandAllocator* allocator) override {
                return std::make_shared<VulkanCommandList>(Device, type);
            }

            // 交换链创建
            virtual IRHISwapChain* CreateSwapChain(
                const SwapChainDesc& desc) override {
                // TODO: 实现交换链创建
                return nullptr;
            }

            // 命令提交和同步
            virtual void SubmitCommandLists(
                uint32 count, IRHICommandList* const* commandLists) override {
                std::vector<VkCommandBuffer> cmdBuffers;
                cmdBuffers.reserve(count);

                for (uint32 i = 0; i < count; ++i) {
                    auto vulkanCmdList =
                        static_cast<VulkanCommandList*>(commandLists[i]);
                    cmdBuffers.push_back(vulkanCmdList->GetCommandBuffer());
                }

                VkSubmitInfo submitInfo = {};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submitInfo.commandBufferCount = count;
                submitInfo.pCommandBuffers = cmdBuffers.data();

                // TODO: 添加信号量和栅栏同步
                vkQueueSubmit(
                    Device->GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
            }

            virtual void WaitForGPU() override {
                vkDeviceWaitIdle(Device->GetHandle());
            }

            virtual void SignalGPU() override {
                // TODO: 实现GPU信号
            }

            // 设备查询
            virtual ERHIFeatureLevel GetFeatureLevel() const override {
                return Device->GetPhysicalDevice()->GetFeatureLevel();
            }

            virtual bool SupportsRayTracing() const override {
                // TODO: 检查光线追踪支持
                return false;
            }

            virtual bool SupportsVariableRateShading() const override {
                // TODO: 检查可变速率着色支持
                return false;
            }

            virtual bool SupportsMeshShaders() const override {
                // TODO: 检查网格着色器支持
                return false;
            }

            // 内存管理
            virtual uint64 GetTotalVideoMemory() const override {
                VkPhysicalDeviceMemoryProperties memProperties;
                vkGetPhysicalDeviceMemoryProperties(
                    Device->GetPhysicalDevice()->GetHandle(), &memProperties);

                uint64 totalMemory = 0;
                for (uint32 i = 0; i < memProperties.memoryHeapCount; ++i) {
                    if (memProperties.memoryHeaps[i].flags &
                        VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                        totalMemory += memProperties.memoryHeaps[i].size;
                    }
                }
                return totalMemory;
            }

            virtual uint64 GetAvailableVideoMemory() const override {
                // TODO: 实现可用显存查询
                return 0;
            }

            // 统计信息
            virtual void GetStats(RHIStats& stats) const override {
                // TODO: 实现统计信息收集
            }

            // 调试支持
            virtual void BeginFrame() override {
                // TODO: 实现帧开始处理
            }

            virtual void EndFrame() override {
                // TODO: 实现帧结束处理
            }

            virtual void SetDebugName(IRHIResource* resource,
                                      const char* name) override {
                if (!resource || !name) {
                    return;
                }

                VkDebugUtilsObjectNameInfoEXT nameInfo = {};
                nameInfo.sType =
                    VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
                nameInfo.pObjectName = name;

                // TODO: 根据资源类型设置对象类型和句柄
                auto func =
                    (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(
                        Device->GetHandle(), "vkSetDebugUtilsObjectNameEXT");
                if (func) {
                    func(Device->GetHandle(), &nameInfo);
                }
            }

          private:
            VulkanRHI* RHI;
            VulkanDevice* Device;
        };

        // 创建RHI设备实例
        RHIDevicePtr CreateRHIDevice(const RHIDeviceCreateParams& params) {
            auto& rhi = VulkanRHI::Get();
            if (!rhi.Initialize(RHIModuleInitParams{params})) {
                LOG_ERROR("Failed to initialize Vulkan RHI!");
                return nullptr;
            }

            return std::make_shared<VulkanDeviceImpl>(&rhi);
        }

    }  // namespace RHI
}  // namespace Engine
