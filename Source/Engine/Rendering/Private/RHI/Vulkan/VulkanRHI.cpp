
#include "VulkanRHI.h"

#include <algorithm>
#include <map>
#include <set>

#include "Core/Public/Log/LogSystem.h"
#ifdef __linux__
#include <xcb/bigreq.h>
#include <xcb/xcb.h>
#endif

#ifdef __linux__
#define VK_USE_PLATFORM_XCB_KHR
#include <vulkan/vulkan_xcb.h>

#endif

#ifndef LOG_VERBOSE
#define LOG_VERBOSE(...) LOG_INFO(__VA_ARGS__)
#endif

namespace Engine {
    namespace RHI {

        namespace {
            const std::vector<const char*> ValidationLayers = {
                "VK_LAYER_KHRONOS_validation"};

            const std::vector<const char*> DeviceExtensions = {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#ifdef BUILD_DEBUG
            const bool EnableValidationLayers = true;
#else
            const bool EnableValidationLayers = false;
#endif
        }  // namespace

        // VulkanInstance Implementation
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
                    LOG_VERBOSE("[Vulkan] %s", pCallbackData->pMessage);
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

        // VulkanPhysicalDevice Implementation
        bool VulkanPhysicalDevice::Initialize(VkInstance instance) {
            Instance = instance;
            if (!SelectPhysicalDevice(instance)) {
                LOG_ERROR("Failed to find a suitable GPU!");
                return false;
            }

            vkGetPhysicalDeviceProperties(PhysicalDevice, &Properties);
            vkGetPhysicalDeviceFeatures(PhysicalDevice, &Features);
            QueueFamilies = FindQueueFamilies(PhysicalDevice);
            FeatureLevel = DetermineFeatureLevel(Properties);

            return true;
        }

        bool VulkanPhysicalDevice::SelectPhysicalDevice(VkInstance instance) {
            uint32_t deviceCount = 0;
            vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

            if (deviceCount == 0) {
                return false;
            }

            std::vector<VkPhysicalDevice> devices(deviceCount);
            vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

            // 按适用性对设备进行排序
            // 使用vector和sort来替代multimap
            std::vector<std::pair<int, VkPhysicalDevice>> candidates;
            for (const auto& device : devices) {
                int score = RateDeviceSuitability(device);
                candidates.push_back(std::make_pair(score, device));
            }

            // 按分数降序排序
            std::sort(
                candidates.begin(),
                candidates.end(),
                [](const auto& a, const auto& b) { return a.first > b.first; });

            // 检查最佳设备是否适用
            if (!candidates.empty() && candidates[0].first > 0) {
                PhysicalDevice = candidates[0].second;
                return true;
            }

            return false;
        }

        int VulkanPhysicalDevice::RateDeviceSuitability(
            VkPhysicalDevice device) const {
            VkPhysicalDeviceProperties deviceProperties;
            VkPhysicalDeviceFeatures deviceFeatures;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);
            vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

            int score = 0;

            // 离散GPU得分较高
            if (deviceProperties.deviceType ==
                VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                score += 1000;
            }

            // 最大纹理大小影响得分
            score += deviceProperties.limits.maxImageDimension2D;

            // 检查必需的特性
            if (!deviceFeatures.geometryShader) {
                return 0;
            }

            // 检查队列族支持
            QueueFamilyIndices indices = FindQueueFamilies(device);
            if (!indices.IsComplete()) {
                return 0;
            }

            return score;
        }

        VulkanPhysicalDevice::QueueFamilyIndices
        VulkanPhysicalDevice::FindQueueFamilies(VkPhysicalDevice device) const {
            QueueFamilyIndices indices;

            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(
                device, &queueFamilyCount, nullptr);

            std::vector<VkQueueFamilyProperties> queueFamilies(
                queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(
                device, &queueFamilyCount, queueFamilies.data());

            for (uint32_t i = 0; i < queueFamilyCount; i++) {
                const auto& queueFamily = queueFamilies[i];

                if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    indices.GraphicsFamily = i;
                }

                if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
                    indices.ComputeFamily = i;
                }

                if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
                    indices.TransferFamily = i;
                }

                if (indices.IsComplete()) {
                    break;
                }
            }

            return indices;
        }

        ERHIFeatureLevel VulkanPhysicalDevice::DetermineFeatureLevel(
            const VkPhysicalDeviceProperties& props) const {
            // 基于Vulkan版本和设备特性确定特性级别
            if (props.apiVersion >= VK_API_VERSION_1_2) {
                return ERHIFeatureLevel::SM6;
            } else if (props.apiVersion >= VK_API_VERSION_1_1) {
                return ERHIFeatureLevel::SM5;
            } else {
                return ERHIFeatureLevel::ES3_1;
            }
        }

        // VulkanDevice Implementation
        VulkanDevice::~VulkanDevice() { Shutdown(); }

        bool VulkanDevice::Initialize(VulkanPhysicalDevice* physicalDevice) {
            PhysicalDevice = physicalDevice;
            return CreateLogicalDevice();
        }

        void VulkanDevice::Shutdown() {
            if (Device != VK_NULL_HANDLE) {
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

            vkGetDeviceQueue(Device, indices.GraphicsFamily, 0, &GraphicsQueue);
            vkGetDeviceQueue(Device, indices.ComputeFamily, 0, &ComputeQueue);
            vkGetDeviceQueue(Device, indices.TransferFamily, 0, &TransferQueue);

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

        // IRHIBuffer* VulkanDevice::CreateBuffer(const BufferDesc& desc) {
        //     // TODO: Implement actual buffer creation
        //     LOG_WARNING("VulkanDevice::CreateBuffer not fully implemented");
        //     return nullptr;
        // }

        // IRHITexture* VulkanDevice::CreateTexture(const TextureDesc& desc) {
        //     // TODO: Implement actual texture creation
        //     LOG_WARNING("VulkanDevice::CreateTexture not fully implemented");
        //     return nullptr;
        // }

        // IRHIShader* VulkanDevice::CreateShader(const ShaderDesc& desc,
        //                                        const void* shaderData,
        //                                        size_t dataSize) {
        //     // TODO: Implement actual shader creation
        //     LOG_WARNING("VulkanDevice::CreateShader not fully implemented");
        //     return nullptr;
        // }

        // IRHIRenderTargetView* VulkanDevice::CreateRenderTargetView(
        //     IRHIResource* resource, const RenderTargetViewDesc& desc) {
        //     // TODO: Implement actual render target view creation
        //     LOG_WARNING(
        //         "VulkanDevice::CreateRenderTargetView not fully
        //         implemented");
        //     return nullptr;
        // }

        // IRHIDepthStencilView* VulkanDevice::CreateDepthStencilView(
        //     IRHIResource* resource, const DepthStencilViewDesc& desc) {
        //     // TODO: Implement actual depth stencil view creation
        //     LOG_WARNING(
        //         "VulkanDevice::CreateDepthStencilView not fully
        //         implemented");
        //     return nullptr;
        // }

        // VulkanCommandPoolManager Implementation
        VulkanCommandPoolManager::~VulkanCommandPoolManager() { Shutdown(); }

        bool VulkanCommandPoolManager::Initialize(VulkanDevice* device) {
            Device = device;
            return CreateCommandPools();
        }

        void VulkanCommandPoolManager::Shutdown() {
            if (Device && Device->GetHandle() != VK_NULL_HANDLE) {
                if (GraphicsCommandPool != VK_NULL_HANDLE) {
                    vkDestroyCommandPool(
                        Device->GetHandle(), GraphicsCommandPool, nullptr);
                    GraphicsCommandPool = VK_NULL_HANDLE;
                }
                if (ComputeCommandPool != VK_NULL_HANDLE) {
                    vkDestroyCommandPool(
                        Device->GetHandle(), ComputeCommandPool, nullptr);
                    ComputeCommandPool = VK_NULL_HANDLE;
                }
                if (TransferCommandPool != VK_NULL_HANDLE) {
                    vkDestroyCommandPool(
                        Device->GetHandle(), TransferCommandPool, nullptr);
                    TransferCommandPool = VK_NULL_HANDLE;
                }
            }
        }

        bool VulkanCommandPoolManager::CreateCommandPools() {
            VkCommandPoolCreateInfo poolInfo = {};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

            // 创建图形命令池
            poolInfo.queueFamilyIndex = Device->GetGraphicsQueueFamilyIndex();
            if (vkCreateCommandPool(Device->GetHandle(),
                                    &poolInfo,
                                    nullptr,
                                    &GraphicsCommandPool) != VK_SUCCESS) {
                LOG_ERROR("Failed to create graphics command pool!");
                return false;
            }

            // 创建计算命令池
            poolInfo.queueFamilyIndex = Device->GetComputeQueueFamilyIndex();
            if (vkCreateCommandPool(Device->GetHandle(),
                                    &poolInfo,
                                    nullptr,
                                    &ComputeCommandPool) != VK_SUCCESS) {
                LOG_ERROR("Failed to create compute command pool!");
                return false;
            }

            // 创建传输命令池
            poolInfo.queueFamilyIndex = Device->GetTransferQueueFamilyIndex();
            if (vkCreateCommandPool(Device->GetHandle(),
                                    &poolInfo,
                                    nullptr,
                                    &TransferCommandPool) != VK_SUCCESS) {
                LOG_ERROR("Failed to create transfer command pool!");
                return false;
            }

            return true;
        }

        // VulkanMemoryAllocator Implementation
        VulkanMemoryAllocator::VulkanMemoryAllocator(
            VkDevice device, VkPhysicalDevice physicalDevice)
            : m_Device(device), m_PhysicalDevice(physicalDevice) {
            vkGetPhysicalDeviceMemoryProperties(physicalDevice,
                                                &m_MemoryProperties);
            m_Stats = {0, 0};
        }

        VulkanMemoryAllocator::~VulkanMemoryAllocator() {
            // 清理所有已分配的内存
            for (auto memory : AllocatedMemory) {
                vkFreeMemory(m_Device, memory, nullptr);
            }
            AllocatedMemory.clear();
        }

        void VulkanMemoryAllocator::Shutdown() {
            // 清理所有已分配的内存
            for (auto memory : AllocatedMemory) {
                if (memory != VK_NULL_HANDLE) {
                    vkFreeMemory(m_Device, memory, nullptr);
                }
            }
            AllocatedMemory.clear();
            m_Stats = {0, 0};
        }

        bool VulkanMemoryAllocator::Initialize(VkDevice device) {
            if (device == VK_NULL_HANDLE) {
                LOG_ERROR(
                    "Invalid device handle provided to "
                    "VulkanMemoryAllocator::Initialize");
                return false;
            }
            m_Device = device;
            return true;
        }

        VkDeviceMemory VulkanMemoryAllocator::AllocateMemory(
            VkMemoryRequirements memRequirements,
            VkMemoryPropertyFlags properties) {
            VkMemoryAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex =
                FindMemoryType(memRequirements.memoryTypeBits, properties);

            VkDeviceMemory memory;
            if (vkAllocateMemory(m_Device, &allocInfo, nullptr, &memory) !=
                VK_SUCCESS) {
                LOG_ERROR("Failed to allocate device memory!");
                return VK_NULL_HANDLE;
            }

            AllocatedMemory.push_back(memory);
            m_Stats.TotalAllocated += memRequirements.size;
            m_Stats.CurrentUsed += memRequirements.size;
            return memory;
        }

        void VulkanMemoryAllocator::FreeMemory(VkDeviceMemory memory) {
            auto it = std::find(
                AllocatedMemory.begin(), AllocatedMemory.end(), memory);
            if (it != AllocatedMemory.end()) {
                vkFreeMemory(m_Device, memory, nullptr);
                AllocatedMemory.erase(it);
            }
        }

        uint32 VulkanMemoryAllocator::FindMemoryType(
            uint32 typeFilter, VkMemoryPropertyFlags properties) {
            for (uint32_t i = 0; i < m_MemoryProperties.memoryTypeCount; i++) {
                if ((typeFilter & (1 << i)) &&
                    (m_MemoryProperties.memoryTypes[i].propertyFlags &
                     properties) == properties) {
                    return i;
                }
            }

            LOG_ERROR("Failed to find suitable memory type!");
            return 0;
        }

        // VulkanRHI Implementation
        VulkanRHI& VulkanRHI::Get() {
            static VulkanRHI instance;
            return instance;
        }

        bool VulkanRHI::Initialize(const RHIModuleInitParams& params) {
            if (!Instance.Initialize(params)) {
                LOG_ERROR("Failed to initialize Vulkan instance!");
                return false;
            }

            if (!PhysicalDevice.Initialize(Instance.GetHandle())) {
                LOG_ERROR("Failed to initialize Vulkan physical device!");
                return false;
            }

            if (!Device.Initialize(&PhysicalDevice)) {
                LOG_ERROR("Failed to initialize Vulkan device!");
                return false;
            }

            if (!CommandPoolManager.Initialize(&Device)) {
                LOG_ERROR("Failed to initialize Vulkan command pool manager!");
                return false;
            }

            if (!MemoryAllocator->Initialize(Device.GetHandle())) {
                LOG_ERROR("Failed to initialize Vulkan memory allocator!");
                return false;
            }

            return true;
        }

        void VulkanRHI::Shutdown() {
            MemoryAllocator->Shutdown();
            CommandPoolManager.Shutdown();
            Device.Shutdown();
            Instance.Shutdown();
        }

    }  // namespace RHI
}  // namespace Engine
