
#include "VulkanDevice.h"

#include <set>

#include "Core/Public/Log/LogSystem.h"
#include "VulkanCommandList.h"
#include "VulkanEvent.h"
#include "VulkanResources.h"
#include "VulkanSamplerState.h"
#include "VulkanShaderFeedback.h"
#include "VulkanSwapChain.h"
#include "VulkanTimelineSemaphore.h"

namespace Engine {
    namespace RHI {

        namespace {
            const std::vector<const char*> DeviceExtensions = {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        }

        VulkanDevice::~VulkanDevice() { Shutdown(); }

        bool VulkanDevice::Initialize(VulkanPhysicalDevice* physicalDevice) {
            PhysicalDevice = physicalDevice;
            return CreateLogicalDevice();
        }

        void VulkanDevice::Shutdown() {
            if (Device != VK_NULL_HANDLE) {
                vkDeviceWaitIdle(Device);
                vkDestroyDevice(Device, nullptr);
                Device = VK_NULL_HANDLE;
            }
        }

        bool VulkanDevice::CreateLogicalDevice() {
            const auto& indices = PhysicalDevice->GetQueueFamilyIndices();

            std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
            std::set<uint32_t> uniqueQueueFamilies = {indices.GraphicsFamily,
                                                      indices.ComputeFamily,
                                                      indices.TransferFamily};

            float queuePriority = 1.0f;
            for (uint32_t queueFamily : uniqueQueueFamilies) {
                VkDeviceQueueCreateInfo queueCreateInfo = {};
                queueCreateInfo.sType =
                    VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueCreateInfo.queueFamilyIndex = queueFamily;
                queueCreateInfo.queueCount = 1;
                queueCreateInfo.pQueuePriorities = &queuePriority;
                queueCreateInfos.push_back(queueCreateInfo);
            }

            VkPhysicalDeviceFeatures deviceFeatures = {};
            deviceFeatures.samplerAnisotropy = VK_TRUE;
            deviceFeatures.fillModeNonSolid = VK_TRUE;
            deviceFeatures.geometryShader = VK_TRUE;

            VkDeviceCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            createInfo.queueCreateInfoCount =
                static_cast<uint32_t>(queueCreateInfos.size());
            createInfo.pQueueCreateInfos = queueCreateInfos.data();
            createInfo.pEnabledFeatures = &deviceFeatures;

            auto extensions = GetRequiredDeviceExtensions();
            createInfo.enabledExtensionCount =
                static_cast<uint32_t>(extensions.size());
            createInfo.ppEnabledExtensionNames = extensions.data();

            if (vkCreateDevice(PhysicalDevice->GetHandle(),
                               &createInfo,
                               nullptr,
                               &Device) != VK_SUCCESS) {
                LOG_ERROR("Failed to create logical device!");
                return false;
            }
            if (Device == VK_NULL_HANDLE) {
                LOG_ERROR("Failed to create logical device!");
                return false;
            }

            VkQueue graphicsQueue, computeQueue, transferQueue;
            vkGetDeviceQueue(Device, indices.GraphicsFamily, 0, &graphicsQueue);
            vkGetDeviceQueue(Device, indices.ComputeFamily, 0, &computeQueue);
            vkGetDeviceQueue(Device, indices.TransferFamily, 0, &transferQueue);
            GraphicsQueue = graphicsQueue;
            ComputeQueue = computeQueue;
            TransferQueue = transferQueue;

            return true;
        }

        std::vector<const char*> VulkanDevice::GetRequiredDeviceExtensions()
            const {
            return DeviceExtensions;
        }

        uint32_t VulkanDevice::GetGraphicsQueueFamilyIndex() const {
            return PhysicalDevice->GetQueueFamilyIndices().GraphicsFamily;
        }

        uint32_t VulkanDevice::GetComputeQueueFamilyIndex() const {
            return PhysicalDevice->GetQueueFamilyIndices().ComputeFamily;
        }

        uint32_t VulkanDevice::GetTransferQueueFamilyIndex() const {
            return PhysicalDevice->GetQueueFamilyIndices().TransferFamily;
        }

        VkFormat VulkanDevice::ConvertToVkFormat(EPixelFormat format) {
            switch (format) {
                case EPixelFormat::D16_UNORM:
                    return VK_FORMAT_D16_UNORM;
                case EPixelFormat::R8G8B8A8_UNORM:
                    return VK_FORMAT_R8G8B8A8_UNORM;
                case EPixelFormat::B8G8R8A8_UNORM:
                    return VK_FORMAT_B8G8R8A8_UNORM;
                case EPixelFormat::R32G32B32A32_FLOAT:
                    return VK_FORMAT_R32G32B32A32_SFLOAT;
                case EPixelFormat::R32G32B32_FLOAT:
                    return VK_FORMAT_R32G32B32_SFLOAT;
                case EPixelFormat::R32G32_FLOAT:
                    return VK_FORMAT_R32G32_SFLOAT;
                case EPixelFormat::R32_FLOAT:
                    return VK_FORMAT_R32_SFLOAT;
                case EPixelFormat::D24_UNORM_S8_UINT:
                    return VK_FORMAT_D24_UNORM_S8_UINT;
                case EPixelFormat::D32_FLOAT:
                    return VK_FORMAT_D32_SFLOAT;
                case EPixelFormat::D32_FLOAT_S8X24_UINT:
                    return VK_FORMAT_D32_SFLOAT_S8_UINT;
                default:
                    return VK_FORMAT_UNDEFINED;
            }
        }

        bool VulkanDevice::IsDepthFormat(EPixelFormat format) const {
            switch (format) {
                case EPixelFormat::D16_UNORM:
                case EPixelFormat::D24_UNORM_S8_UINT:
                case EPixelFormat::D32_FLOAT:
                case EPixelFormat::D32_FLOAT_S8X24_UINT:
                    return true;
                default:
                    return false;
            }
        }

        bool VulkanDevice::IsStencilFormat(EPixelFormat format) const {
            switch (format) {
                case EPixelFormat::D24_UNORM_S8_UINT:
                case EPixelFormat::D32_FLOAT_S8X24_UINT:
                    return true;
                default:
                    return false;
            }
        }

        // 资源创建方法的实现...
        // 注意：这些方法的具体实现已经在原始文件中，这里为了简洁省略了具体实现
        // 实际使用时需要将原始文件中的相应实现复制到这里

        void VulkanDevice::SubmitCommandLists(
            uint32 count, IRHICommandList* const* commandLists) {
            std::vector<VkCommandBuffer> vulkanCommandBuffers;
            vulkanCommandBuffers.reserve(count);

            for (uint32 i = 0; i < count; ++i) {
                auto vulkanCmdList =
                    dynamic_cast<VulkanCommandList*>(commandLists[i]);
                if (vulkanCmdList) {
                    vulkanCommandBuffers.push_back(
                        vulkanCmdList->GetCommandBuffer());
                }
            }

            VkSubmitInfo submitInfo = {};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount =
                static_cast<uint32_t>(vulkanCommandBuffers.size());
            submitInfo.pCommandBuffers = vulkanCommandBuffers.data();

            if (vkQueueSubmit(GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) !=
                VK_SUCCESS) {
                LOG_ERROR("Failed to submit command buffers!");
            }
        }

        void VulkanDevice::WaitForGPU() { vkDeviceWaitIdle(Device); }

        void VulkanDevice::SignalGPU() {
            VkFenceCreateInfo fenceInfo = {};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

            VkFence fence;
            if (vkCreateFence(Device, &fenceInfo, nullptr, &fence) !=
                VK_SUCCESS) {
                LOG_ERROR("Failed to create fence for GPU signaling!");
                return;
            }

            VkSubmitInfo submitInfo = {};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

            if (vkQueueSubmit(GraphicsQueue, 1, &submitInfo, fence) !=
                VK_SUCCESS) {
                LOG_ERROR(
                    "Failed to submit empty command buffer for GPU signaling!");
                vkDestroyFence(Device, fence, nullptr);
                return;
            }

            vkWaitForFences(Device, 1, &fence, VK_TRUE, UINT64_MAX);
            vkDestroyFence(Device, fence, nullptr);
        }

        ERHIFeatureLevel VulkanDevice::GetFeatureLevel() const {
            return PhysicalDevice->GetFeatureLevel();
        }

        void VulkanDevice::SetDebugName(IRHIResource* resource,
                                        const char* name) {
            if (!resource || !name) {
                return;
            }

            uint64_t handle = 0;
            VkObjectType objectType = VK_OBJECT_TYPE_UNKNOWN;

            if (auto buffer = dynamic_cast<VulkanBuffer*>(resource)) {
                handle = (uint64_t)buffer->GetHandle();
                objectType = VK_OBJECT_TYPE_BUFFER;
            } else if (auto texture = dynamic_cast<VulkanTexture*>(resource)) {
                handle = (uint64_t)texture->GetHandle();
                objectType = VK_OBJECT_TYPE_IMAGE;
            } else if (auto sampler =
                           dynamic_cast<VulkanSamplerState*>(resource)) {
                handle = (uint64_t)sampler->GetHandle();
                objectType = VK_OBJECT_TYPE_SAMPLER;
            }

            if (handle && objectType != VK_OBJECT_TYPE_UNKNOWN) {
                SetDebugName(objectType, handle, name);
            }
        }

        void VulkanDevice::SetDebugName(VkObjectType objectType,
                                        uint64_t object,
                                        const char* name) {
            if (!name || object == 0) {
                return;
            }

            VkDebugUtilsObjectNameInfoEXT nameInfo = {};
            nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            nameInfo.objectType = objectType;
            nameInfo.objectHandle = object;
            nameInfo.pObjectName = name;

            auto func = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(
                Device, "vkSetDebugUtilsObjectNameEXT");
            if (func) {
                func(Device, &nameInfo);
            }
        }

    }  // namespace RHI
}  // namespace Engine
