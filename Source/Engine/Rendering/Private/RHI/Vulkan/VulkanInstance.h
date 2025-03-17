
#pragma once

#include <vulkan/vulkan.h>

#include <vector>

#include "Core/Public/Log/LogSystem.h"
#include "RHI/RHIModule.h"

namespace Engine {
    namespace RHI {

        class VulkanInstance {
          public:
            ~VulkanInstance();

            bool Initialize(const RHIModuleInitParams& params);
            void Shutdown();
            VkInstance GetHandle() const { return Instance; }

          private:
            bool CreateInstance(const RHIModuleInitParams& params);
            bool SetupDebugMessenger();
            std::vector<const char*> GetRequiredExtensions() const;
            bool CheckValidationLayerSupport() const;

            static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
                VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                void* pUserData);

          private:
            VkInstance Instance = VK_NULL_HANDLE;
            VkDebugUtilsMessengerEXT DebugMessenger = VK_NULL_HANDLE;
            bool ValidationLayersEnabled = false;
        };

    }  // namespace RHI
}  // namespace Engine
