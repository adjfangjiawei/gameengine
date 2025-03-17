
#include "VulkanInstance.h"

#include "Log/LogSystem.h"

#ifdef __WINDOWS__
#include <Windows.h>
#endif

#ifdef __linux__
#include <xcb/xcb.h>
#endif

#ifdef __linux__
#include <vulkan/vulkan_xcb.h>

#endif

namespace Engine {
    namespace RHI {

        namespace {
            const std::vector<const char*> ValidationLayers = {
                "VK_LAYER_KHRONOS_validation"};

#ifdef BUILD_DEBUG
            const bool EnableValidationLayers = true;
#else
            const bool EnableValidationLayers = false;
#endif
        }  // namespace

        VulkanInstance::~VulkanInstance() { Shutdown(); }

        bool VulkanInstance::Initialize(const RHIModuleInitParams& params) {
            ValidationLayersEnabled =
                EnableValidationLayers && params.DeviceParams.EnableDebugLayer;

            if (!CreateInstance(params)) {
                return false;
            }

            if (ValidationLayersEnabled) {
                if (!SetupDebugMessenger()) {
                    return false;
                }
            }

            return true;
        }

        void VulkanInstance::Shutdown() {
            if (DebugMessenger != VK_NULL_HANDLE) {
                auto func =
                    (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
                        Instance, "vkDestroyDebugUtilsMessengerEXT");
                if (func != nullptr) {
                    func(Instance, DebugMessenger, nullptr);
                }
                DebugMessenger = VK_NULL_HANDLE;
            }

            if (Instance != VK_NULL_HANDLE) {
                vkDestroyInstance(Instance, nullptr);
                Instance = VK_NULL_HANDLE;
            }
        }

        bool VulkanInstance::CreateInstance(const RHIModuleInitParams& params) {
            if (ValidationLayersEnabled && !CheckValidationLayerSupport()) {
                LOG_ERROR("Validation layers requested, but not available!");
                return false;
            }

            VkApplicationInfo appInfo = {};
            appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            appInfo.pApplicationName =
                params.DeviceParams.ApplicationName.c_str();
            appInfo.applicationVersion = params.DeviceParams.ApplicationVersion;
            appInfo.pEngineName = "Engine";
            appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.apiVersion = VK_API_VERSION_1_2;

            VkInstanceCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            createInfo.pApplicationInfo = &appInfo;

            auto extensions = GetRequiredExtensions();
            createInfo.enabledExtensionCount =
                static_cast<uint32_t>(extensions.size());
            createInfo.ppEnabledExtensionNames = extensions.data();

            VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
            if (ValidationLayersEnabled) {
                createInfo.enabledLayerCount =
                    static_cast<uint32_t>(ValidationLayers.size());
                createInfo.ppEnabledLayerNames = ValidationLayers.data();

                debugCreateInfo.sType =
                    VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
                debugCreateInfo.messageSeverity =
                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
                debugCreateInfo.messageType =
                    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
                debugCreateInfo.pfnUserCallback = DebugCallback;
                createInfo.pNext = &debugCreateInfo;
            } else {
                createInfo.enabledLayerCount = 0;
                createInfo.pNext = nullptr;
            }

            if (vkCreateInstance(&createInfo, nullptr, &Instance) !=
                VK_SUCCESS) {
                LOG_ERROR("Failed to create Vulkan instance!");
                return false;
            }

            return true;
        }

        bool VulkanInstance::SetupDebugMessenger() {
            VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
            createInfo.sType =
                VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            createInfo.messageSeverity =
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            createInfo.messageType =
                VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            createInfo.pfnUserCallback = DebugCallback;

            auto func =
                (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
                    Instance, "vkCreateDebugUtilsMessengerEXT");
            if (func == nullptr ||
                func(Instance, &createInfo, nullptr, &DebugMessenger) !=
                    VK_SUCCESS) {
                LOG_ERROR("Failed to set up debug messenger!");
                return false;
            }

            return true;
        }

        std::vector<const char*> VulkanInstance::GetRequiredExtensions() const {
            std::vector<const char*> extensions;

            // 添加必需的扩展
            extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

#ifdef _WIN32
            extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(__linux__)
            extensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#endif

            if (ValidationLayersEnabled) {
                extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }

            return extensions;
        }

        bool VulkanInstance::CheckValidationLayerSupport() const {
            uint32_t layerCount;
            vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

            std::vector<VkLayerProperties> availableLayers(layerCount);
            vkEnumerateInstanceLayerProperties(&layerCount,
                                               availableLayers.data());

            for (const char* layerName : ValidationLayers) {
                bool layerFound = false;

                for (const auto& layerProperties : availableLayers) {
                    if (strcmp(layerName, layerProperties.layerName) == 0) {
                        layerFound = true;
                        break;
                    }
                }

                if (!layerFound) {
                    return false;
                }
            }

            return true;
        }

        VKAPI_ATTR VkBool32 VKAPI_CALL VulkanInstance::DebugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            [[maybe_unused]] void* pUserData) {
            switch (messageSeverity) {
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
                    LOG_INFO("[Vulkan] %s", pCallbackData->pMessage);
                    break;
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
                    LOG_INFO("[Vulkan] %s", pCallbackData->pMessage);
                    break;
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                    LOG_WARNING("[Vulkan] %s", pCallbackData->pMessage);
                    break;
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                    LOG_ERROR("[Vulkan] %s", pCallbackData->pMessage);
                    break;
                default:
                    LOG_INFO("[Vulkan] %s", pCallbackData->pMessage);
                    break;
            }

            return VK_FALSE;
        }

    }  // namespace RHI
}  // namespace Engine
