#include "VulkanSwapChain.h"

#include "Core/Log/LogSystem.h"

namespace Engine {
    namespace RHI {

        VulkanSwapChain::VulkanSwapChain(VulkanDevice* device,
                                         const SwapChainDesc& desc)
            : Device(device),
              Desc(desc),
              CurrentState(ERHIResourceState::Present),
              SwapChain(VK_NULL_HANDLE),
              Surface(VK_NULL_HANDLE),
              SwapChainFormat(VK_FORMAT_UNDEFINED),
              CurrentImageIndex(0) {
            if (!Initialize()) {
                LOG_ERROR("Failed to initialize swap chain!");
            }
        }

        VulkanSwapChain::~VulkanSwapChain() { Cleanup(); }

        bool VulkanSwapChain::Initialize() {
            if (!CreateSurface()) {
                return false;
            }

            if (!CreateSwapChain()) {
                return false;
            }

            if (!CreateImageViews()) {
                return false;
            }

            return true;
        }

        void VulkanSwapChain::Cleanup() {
            DestroyImageViews();

            if (SwapChain != VK_NULL_HANDLE) {
                vkDestroySwapchainKHR(Device->GetHandle(), SwapChain, nullptr);
                SwapChain = VK_NULL_HANDLE;
            }

            if (Surface != VK_NULL_HANDLE) {
                vkDestroySurfaceKHR(
                    Device->GetPhysicalDevice()->GetHandle(), Surface, nullptr);
                Surface = VK_NULL_HANDLE;
            }
        }

        bool VulkanSwapChain::CreateSurface() {
#ifdef _WIN32
            VkWin32SurfaceCreateInfoKHR createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
            createInfo.hwnd = static_cast<HWND>(Desc.WindowHandle);
            createInfo.hinstance = GetModuleHandle(nullptr);

            if (vkCreateWin32SurfaceKHR(
                    Device->GetPhysicalDevice()->GetHandle(),
                    &createInfo,
                    nullptr,
                    &Surface) != VK_SUCCESS) {
                LOG_ERROR("Failed to create window surface!");
                return false;
            }
#elif defined(__linux__)
            VkXcbSurfaceCreateInfoKHR createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
            // TODO: 设置XCB特定的参数

            if (vkCreateXcbSurfaceKHR(Device->GetPhysicalDevice()->GetHandle(),
                                      &createInfo,
                                      nullptr,
                                      &Surface) != VK_SUCCESS) {
                LOG_ERROR("Failed to create window surface!");
                return false;
            }
#endif

            return true;
        }

        bool VulkanSwapChain::CreateSwapChain() {
            SwapChainSupportDetails swapChainSupport =
                QuerySwapChainSupport(Device->GetPhysicalDevice()->GetHandle());

            VkSurfaceFormatKHR surfaceFormat =
                ChooseSwapSurfaceFormat(swapChainSupport.Formats);
            VkPresentModeKHR presentMode =
                ChooseSwapPresentMode(swapChainSupport.PresentModes);
            VkExtent2D extent = ChooseSwapExtent(swapChainSupport.Capabilities);

            uint32_t imageCount =
                swapChainSupport.Capabilities.minImageCount + 1;
            if (swapChainSupport.Capabilities.maxImageCount > 0 &&
                imageCount > swapChainSupport.Capabilities.maxImageCount) {
                imageCount = swapChainSupport.Capabilities.maxImageCount;
            }

            VkSwapchainCreateInfoKHR createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            createInfo.surface = Surface;
            createInfo.minImageCount = imageCount;
            createInfo.imageFormat = surfaceFormat.format;
            createInfo.imageColorSpace = surfaceFormat.colorSpace;
            createInfo.imageExtent = extent;
            createInfo.imageArrayLayers = 1;
            createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

            uint32_t queueFamilyIndices[] = {
                Device->GetGraphicsQueueFamilyIndex(),
                Device->GetPresentQueueFamilyIndex()};

            if (queueFamilyIndices[0] != queueFamilyIndices[1]) {
                createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                createInfo.queueFamilyIndexCount = 2;
                createInfo.pQueueFamilyIndices = queueFamilyIndices;
            } else {
                createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            }

            createInfo.preTransform =
                swapChainSupport.Capabilities.currentTransform;
            createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            createInfo.presentMode = presentMode;
            createInfo.clipped = VK_TRUE;
            createInfo.oldSwapchain = VK_NULL_HANDLE;

            if (vkCreateSwapchainKHR(
                    Device->GetHandle(), &createInfo, nullptr, &SwapChain) !=
                VK_SUCCESS) {
                LOG_ERROR("Failed to create swap chain!");
                return false;
            }

            vkGetSwapchainImagesKHR(
                Device->GetHandle(), SwapChain, &imageCount, nullptr);
            SwapChainImages.resize(imageCount);
            vkGetSwapchainImagesKHR(Device->GetHandle(),
                                    SwapChain,
                                    &imageCount,
                                    SwapChainImages.data());

            SwapChainFormat = surfaceFormat.format;
            SwapChainExtent = extent;

            return true;
        }

        bool VulkanSwapChain::CreateImageViews() {
            SwapChainImageViews.resize(SwapChainImages.size());
            BackBuffers.resize(SwapChainImages.size());

            for (size_t i = 0; i < SwapChainImages.size(); i++) {
                VkImageViewCreateInfo createInfo = {};
                createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                createInfo.image = SwapChainImages[i];
                createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                createInfo.format = SwapChainFormat;
                createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.subresourceRange.aspectMask =
                    VK_IMAGE_ASPECT_COLOR_BIT;
                createInfo.subresourceRange.baseMipLevel = 0;
                createInfo.subresourceRange.levelCount = 1;
                createInfo.subresourceRange.baseArrayLayer = 0;
                createInfo.subresourceRange.layerCount = 1;

                if (vkCreateImageView(Device->GetHandle(),
                                      &createInfo,
                                      nullptr,
                                      &SwapChainImageViews[i]) != VK_SUCCESS) {
                    LOG_ERROR("Failed to create image views!");
                    return false;
                }

                // 创建后备缓冲区包装器
                TextureDesc backBufferDesc;
                backBufferDesc.Width = SwapChainExtent.width;
                backBufferDesc.Height = SwapChainExtent.height;
                backBufferDesc.Format = Desc.Format;
                backBufferDesc.Dimension = ERHIResourceDimension::Texture2D;
                backBufferDesc.DebugName =
                    Desc.DebugName + "_BackBuffer" + std::to_string(i);

                // TODO: 创建后备缓冲区包装器
                // BackBuffers[i] = std::make_unique<VulkanTexture>(Device,
                // backBufferDesc, SwapChainImages[i]);
            }

            return true;
        }

        void VulkanSwapChain::DestroyImageViews() {
            BackBuffers.clear();

            for (auto imageView : SwapChainImageViews) {
                vkDestroyImageView(Device->GetHandle(), imageView, nullptr);
            }
            SwapChainImageViews.clear();
        }

        VkSurfaceFormatKHR VulkanSwapChain::ChooseSwapSurfaceFormat(
            const std::vector<VkSurfaceFormatKHR>& availableFormats) {
            for (const auto& availableFormat : availableFormats) {
                if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
                    availableFormat.colorSpace ==
                        VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                    return availableFormat;
                }
            }

            return availableFormats[0];
        }

        VkPresentModeKHR VulkanSwapChain::ChooseSwapPresentMode(
            const std::vector<VkPresentModeKHR>& availablePresentModes) {
            if (Desc.VSync) {
                return VK_PRESENT_MODE_FIFO_KHR;
            }

            for (const auto& availablePresentMode : availablePresentModes) {
                if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                    return availablePresentMode;
                }
            }

            return VK_PRESENT_MODE_FIFO_KHR;
        }

        VkExtent2D VulkanSwapChain::ChooseSwapExtent(
            const VkSurfaceCapabilitiesKHR& capabilities) {
            if (capabilities.currentExtent.width != UINT32_MAX) {
                return capabilities.currentExtent;
            }

            VkExtent2D actualExtent = {static_cast<uint32_t>(Desc.Width),
                                       static_cast<uint32_t>(Desc.Height)};

            actualExtent.width =
                std::max(capabilities.minImageExtent.width,
                         std::min(capabilities.maxImageExtent.width,
                                  actualExtent.width));
            actualExtent.height =
                std::max(capabilities.minImageExtent.height,
                         std::min(capabilities.maxImageExtent.height,
                                  actualExtent.height));

            return actualExtent;
        }

        VulkanSwapChain::SwapChainSupportDetails
        VulkanSwapChain::QuerySwapChainSupport(VkPhysicalDevice device) {
            SwapChainSupportDetails details;

            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                device, Surface, &details.Capabilities);

            uint32_t formatCount;
            vkGetPhysicalDeviceSurfaceFormatsKHR(
                device, Surface, &formatCount, nullptr);
            if (formatCount != 0) {
                details.Formats.resize(formatCount);
                vkGetPhysicalDeviceSurfaceFormatsKHR(
                    device, Surface, &formatCount, details.Formats.data());
            }

            uint32_t presentModeCount;
            vkGetPhysicalDeviceSurfacePresentModesKHR(
                device, Surface, &presentModeCount, nullptr);
            if (presentModeCount != 0) {
                details.PresentModes.resize(presentModeCount);
                vkGetPhysicalDeviceSurfacePresentModesKHR(
                    device,
                    Surface,
                    &presentModeCount,
                    details.PresentModes.data());
            }

            return details;
        }

        IRHITexture* VulkanSwapChain::GetBackBuffer(uint32 index) {
            if (index >= BackBuffers.size()) {
                return nullptr;
            }
            return BackBuffers[index].get();
        }

        uint32 VulkanSwapChain::GetCurrentBackBufferIndex() const {
            return CurrentImageIndex;
        }

        void VulkanSwapChain::Present() {
            VkPresentInfoKHR presentInfo = {};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = &SwapChain;
            presentInfo.pImageIndices = &CurrentImageIndex;

            vkQueuePresentKHR(Device->GetPresentQueue(), &presentInfo);
        }

        void VulkanSwapChain::Resize(uint32 width, uint32 height) {
            if (width == 0 || height == 0) {
                return;
            }

            Desc.Width = width;
            Desc.Height = height;

            // 重新创建交换链
            Cleanup();
            Initialize();
        }

        uint64 VulkanSwapChain::GetSize() const {
            return static_cast<uint64>(SwapChainExtent.width) *
                   static_cast<uint64>(SwapChainExtent.height) *
                   4;  // 假设每像素4字节
        }

        // SwapChainSyncObjects Implementation
        SwapChainSyncObjects::SwapChainSyncObjects(VkDevice device) {
            ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
            RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
            InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

            VkSemaphoreCreateInfo semaphoreInfo = {};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            VkFenceCreateInfo fenceInfo = {};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                if (vkCreateSemaphore(device,
                                      &semaphoreInfo,
                                      nullptr,
                                      &ImageAvailableSemaphores[i]) !=
                        VK_SUCCESS ||
                    vkCreateSemaphore(device,
                                      &semaphoreInfo,
                                      nullptr,
                                      &RenderFinishedSemaphores[i]) !=
                        VK_SUCCESS ||
                    vkCreateFence(
                        device, &fenceInfo, nullptr, &InFlightFences[i]) !=
                        VK_SUCCESS) {
                    LOG_ERROR(
                        "Failed to create synchronization objects for a "
                        "frame!");
                    return;
                }
            }
        }

        SwapChainSyncObjects::~SwapChainSyncObjects() {
            // 清理工作应该由外部调用Cleanup完成
        }

        void SwapChainSyncObjects::Cleanup(VkDevice device) {
            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                vkDestroySemaphore(
                    device, RenderFinishedSemaphores[i], nullptr);
                vkDestroySemaphore(
                    device, ImageAvailableSemaphores[i], nullptr);
                vkDestroyFence(device, InFlightFences[i], nullptr);
            }
        }

    }  // namespace RHI
}  // namespace Engine
e