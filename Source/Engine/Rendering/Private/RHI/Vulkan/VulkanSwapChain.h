
#pragma once

#include <vector>

#include "VulkanRHI.h"

namespace Engine {
    namespace RHI {

        class VulkanSwapChain : public IRHISwapChain {
            friend class VulkanRHI;
            friend class VulkanDevice;

          public:
            VulkanSwapChain(VulkanDevice* device, const SwapChainDesc& desc);
            virtual ~VulkanSwapChain();

            // IRHISwapChain接口实现
            virtual IRHITexture* GetBackBuffer(uint32 index) override;
            virtual uint32 GetCurrentBackBufferIndex() const override;
            virtual void Present() override;
            virtual void Resize(uint32 width, uint32 height) override;
            virtual const SwapChainDesc& GetDesc() const override {
                return Desc;
            }

            // IRHIResource接口实现
            virtual const std::string& GetName() const override {
                return Desc.DebugName;
            }
            virtual ERHIResourceState GetCurrentState() const override {
                return CurrentState;
            }
            virtual ERHIResourceDimension GetResourceDimension()
                const override {
                return ERHIResourceDimension::Texture2D;
            }
            virtual uint64 GetSize() const override;

            // Vulkan特定方法
            VkSwapchainKHR GetHandle() const { return SwapChain; }
            VkSurfaceKHR GetSurface() const { return Surface; }
            VkFormat GetFormat() const { return SwapChainFormat; }
            const std::vector<VkImage>& GetImages() const {
                return SwapChainImages;
            }
            const std::vector<VkImageView>& GetImageViews() const {
                return SwapChainImageViews;
            }

          private:
            bool Initialize();
            void Cleanup();
            bool CreateSurface();
            bool CreateSwapChain();
            bool CreateImageViews();
            void DestroyImageViews();
            VkSurfaceFormatKHR ChooseSwapSurfaceFormat(
                const std::vector<VkSurfaceFormatKHR>& availableFormats);
            VkPresentModeKHR ChooseSwapPresentMode(
                const std::vector<VkPresentModeKHR>& availablePresentModes);
            VkExtent2D ChooseSwapExtent(
                const VkSurfaceCapabilitiesKHR& capabilities);

            VulkanDevice* Device;
            SwapChainDesc Desc;
            ERHIResourceState CurrentState;

            VkSwapchainKHR SwapChain;
            VkSurfaceKHR Surface;
            VkFormat SwapChainFormat;
            VkExtent2D SwapChainExtent;
            std::vector<VkImage> SwapChainImages;
            std::vector<VkImageView> SwapChainImageViews;
            std::vector<std::unique_ptr<class VulkanTexture>> BackBuffers;
            uint32 CurrentImageIndex;

            struct SwapChainSupportDetails {
                VkSurfaceCapabilitiesKHR Capabilities;
                std::vector<VkSurfaceFormatKHR> Formats;
                std::vector<VkPresentModeKHR> PresentModes;
            };
            SwapChainSupportDetails QuerySwapChainSupport(
                VkPhysicalDevice device);
        };

        // 交换链同步对象
        struct SwapChainSyncObjects {
            std::vector<VkSemaphore> ImageAvailableSemaphores;
            std::vector<VkSemaphore> RenderFinishedSemaphores;
            std::vector<VkFence> InFlightFences;
            std::vector<VkFence> ImagesInFlight;
            size_t CurrentFrame = 0;

            static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

            SwapChainSyncObjects(VkDevice device);
            ~SwapChainSyncObjects();

            void Cleanup(VkDevice device);
            VkSemaphore GetCurrentImageAvailableSemaphore() const {
                return ImageAvailableSemaphores[CurrentFrame];
            }
            VkSemaphore GetCurrentRenderFinishedSemaphore() const {
                return RenderFinishedSemaphores[CurrentFrame];
            }
            VkFence GetCurrentInFlightFence() const {
                return InFlightFences[CurrentFrame];
            }
            void AdvanceFrame() {
                CurrentFrame = (CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
            }
        };

    }  // namespace RHI
}  // namespace Engine
