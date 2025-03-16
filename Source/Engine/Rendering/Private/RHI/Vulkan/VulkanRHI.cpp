
#include "VulkanRHI.h"

#include <algorithm>
#include <map>
#include <set>

#include "Core/Public/Log/LogSystem.h"
#include "VulkanCommandList.h"
#include "VulkanEvent.h"
#include "VulkanMeshShader.h"
#include "VulkanResources.h"
#include "VulkanSamplerState.h"
#include "VulkanShaderFeedback.h"
#include "VulkanSwapChain.h"
#include "VulkanTimelineSemaphore.h"
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

            // 创建内存分配器
            MemoryAllocator = std::make_unique<VulkanMemoryAllocator>(
                VK_NULL_HANDLE, PhysicalDevice);
            if (!MemoryAllocator) {
                LOG_ERROR("Failed to create memory allocator!");
                return false;
            }

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

        // 将RHI像素格式转换为Vulkan格式
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

        IRHIBuffer* VulkanDevice::CreateBuffer(const BufferDesc& desc) {
            VkBufferCreateInfo bufferInfo = {};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = desc.SizeInBytes;
            bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                               VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            // 根据访问标志设置内存属性
            VkMemoryPropertyFlags memoryProperties = 0;
            if (static_cast<uint32>(desc.Access) &
                static_cast<uint32>(ERHIAccessFlags::CPUWrite)) {
                memoryProperties |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
                memoryProperties |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            }
            if (static_cast<uint32>(desc.Access) &
                static_cast<uint32>(ERHIAccessFlags::GPURead)) {
                memoryProperties |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            }

            VkBuffer buffer;
            if (vkCreateBuffer(Device, &bufferInfo, nullptr, &buffer) !=
                VK_SUCCESS) {
                LOG_ERROR("Failed to create buffer!");
                return nullptr;
            }

            // 获取内存需求
            VkMemoryRequirements memRequirements;
            vkGetBufferMemoryRequirements(Device, buffer, &memRequirements);

            // 分配内存
            VkDeviceMemory bufferMemory =
                PhysicalDevice->GetMemoryAllocator()->AllocateMemory(
                    memRequirements, memoryProperties);

            if (bufferMemory == VK_NULL_HANDLE) {
                vkDestroyBuffer(Device, buffer, nullptr);
                LOG_ERROR("Failed to allocate buffer memory!");
                return nullptr;
            }

            // 绑定内存到缓冲区
            if (vkBindBufferMemory(Device, buffer, bufferMemory, 0) !=
                VK_SUCCESS) {
                vkDestroyBuffer(Device, buffer, nullptr);
                PhysicalDevice->GetMemoryAllocator()->FreeMemory(bufferMemory);
                LOG_ERROR("Failed to bind buffer memory!");
                return nullptr;
            }

            // 创建VulkanBuffer对象并初始化
            VulkanBuffer* vulkanBuffer = new VulkanBuffer(this, desc);
            if (vulkanBuffer) {
                vulkanBuffer->SetHandle(buffer, bufferMemory);
            }
            return vulkanBuffer;
        }

        IRHITexture* VulkanDevice::CreateTexture(const TextureDesc& desc) {
            VkImageCreateInfo imageInfo = {};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width = desc.Width;
            imageInfo.extent.height = desc.Height;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.format =
                VK_FORMAT_R8G8B8A8_UNORM;  // 默认格式，需要根据desc.Format转换
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage =
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

            // 根据flags设置用途
            if (static_cast<uint32_t>(desc.Flags) &
                static_cast<uint32_t>(ERHIResourceFlags::AllowRenderTarget)) {
                imageInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            }
            if (static_cast<uint32_t>(desc.Flags) &
                static_cast<uint32_t>(ERHIResourceFlags::AllowDepthStencil)) {
                imageInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
                imageInfo.format = VK_FORMAT_D24_UNORM_S8_UINT;  // 深度模板格式
            }

            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.flags = 0;

            VkImage image;
            if (vkCreateImage(Device, &imageInfo, nullptr, &image) !=
                VK_SUCCESS) {
                LOG_ERROR("Failed to create image!");
                return nullptr;
            }

            // 获取内存需求
            VkMemoryRequirements memRequirements;
            vkGetImageMemoryRequirements(Device, image, &memRequirements);

            // 分配内存
            VkDeviceMemory imageMemory =
                PhysicalDevice->GetMemoryAllocator()->AllocateMemory(
                    memRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            if (imageMemory == VK_NULL_HANDLE) {
                vkDestroyImage(Device, image, nullptr);
                LOG_ERROR("Failed to allocate image memory!");
                return nullptr;
            }

            // 绑定内存到图像
            if (vkBindImageMemory(Device, image, imageMemory, 0) !=
                VK_SUCCESS) {
                vkDestroyImage(Device, image, nullptr);
                PhysicalDevice->GetMemoryAllocator()->FreeMemory(imageMemory);
                LOG_ERROR("Failed to bind image memory!");
                return nullptr;
            }

            // 创建图像视图
            VkImageViewCreateInfo viewInfo = {};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = image;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = imageInfo.format;
            viewInfo.subresourceRange.aspectMask =
                (static_cast<uint32_t>(desc.Flags) &
                 static_cast<uint32_t>(ERHIResourceFlags::AllowDepthStencil))
                    ? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT
                    : VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            VkImageView imageView;
            if (vkCreateImageView(Device, &viewInfo, nullptr, &imageView) !=
                VK_SUCCESS) {
                vkDestroyImage(Device, image, nullptr);
                PhysicalDevice->GetMemoryAllocator()->FreeMemory(imageMemory);
                LOG_ERROR("Failed to create texture image view!");
                return nullptr;
            }

            // 创建VulkanTexture对象并初始化
            VulkanTexture* vulkanTexture = new VulkanTexture(this, desc);
            if (vulkanTexture) {
                vulkanTexture->SetHandle(image, imageMemory, imageView);
            }
            return vulkanTexture;
        }

        IRHIShader* VulkanDevice::CreateShader(const ShaderDesc& desc,
                                               const void* shaderData,
                                               size_t dataSize) {
            VkShaderModuleCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createInfo.codeSize = dataSize;
            createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderData);

            VkShaderModule shaderModule;
            if (vkCreateShaderModule(
                    Device, &createInfo, nullptr, &shaderModule) !=
                VK_SUCCESS) {
                LOG_ERROR("Failed to create shader module!");
                return nullptr;
            }

            // 创建着色器阶段信息
            VkShaderStageFlagBits shaderStage;
            switch (desc.Type) {
                case EShaderType::Vertex:
                    shaderStage = VK_SHADER_STAGE_VERTEX_BIT;
                    break;
                case EShaderType::Pixel:
                    shaderStage = VK_SHADER_STAGE_FRAGMENT_BIT;
                    break;
                case EShaderType::Compute:
                    shaderStage = VK_SHADER_STAGE_COMPUTE_BIT;
                    break;
                case EShaderType::Geometry:
                    shaderStage = VK_SHADER_STAGE_GEOMETRY_BIT;
                    break;
                default:
                    LOG_ERROR("Unsupported shader type!");
                    vkDestroyShaderModule(Device, shaderModule, nullptr);
                    return nullptr;
            }

            VkPipelineShaderStageCreateInfo shaderStageInfo = {};
            shaderStageInfo.sType =
                VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStageInfo.stage = shaderStage;
            shaderStageInfo.module = shaderModule;
            shaderStageInfo.pName = desc.EntryPoint.c_str();

            // 创建VulkanShader对象并初始化
            VulkanShader* vulkanShader = new VulkanShader(this, desc);
            if (vulkanShader) {
                vulkanShader->SetHandle(shaderModule, shaderStageInfo);
            }
            return vulkanShader;
        }

        IRHIRenderTargetView* VulkanDevice::CreateRenderTargetView(
            IRHIResource* resource, const RenderTargetViewDesc& desc) {
            if (!resource) {
                LOG_ERROR(
                    "Invalid resource provided for render target view "
                    "creation!");
                return nullptr;
            }

            VkImage targetImage = VK_NULL_HANDLE;

            // 获取目标图像
            auto texture = dynamic_cast<VulkanTexture*>(resource);
            if (!texture) {
                LOG_ERROR("Unsupported resource type for render target view!");
                return nullptr;
            }
            targetImage = texture->GetHandle();

            if (targetImage == VK_NULL_HANDLE) {
                LOG_ERROR("Failed to get target image for render target view!");
                return nullptr;
            }

            // 创建图像视图
            VkImageViewCreateInfo viewInfo = {};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = targetImage;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = VulkanDevice::ConvertToVkFormat(desc.Format);
            viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            VkImageView imageView;
            if (vkCreateImageView(Device, &viewInfo, nullptr, &imageView) !=
                VK_SUCCESS) {
                LOG_ERROR("Failed to create render target image view!");
                return nullptr;
            }

            // 创建渲染目标视图描述符
            VkDescriptorImageInfo descriptorInfo = {};
            descriptorInfo.imageLayout =
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            descriptorInfo.imageView = imageView;
            descriptorInfo.sampler = VK_NULL_HANDLE;  // RTV不需要采样器

            // 创建VulkanRenderTargetView对象并初始化
            VulkanRenderTargetView* vulkanRTV =
                new VulkanRenderTargetView(this, desc);
            if (vulkanRTV) {
                vulkanRTV->SetResource(resource);
                vulkanRTV->SetHandle(imageView, descriptorInfo);
            }
            return vulkanRTV;
        }

        IRHIDepthStencilView* VulkanDevice::CreateDepthStencilView(
            IRHIResource* resource, const DepthStencilViewDesc& desc) {
            if (!resource) {
                LOG_ERROR(
                    "Invalid resource provided for depth stencil view "
                    "creation!");
                return nullptr;
            }

            // 获取目标图像
            VkImage targetImage = VK_NULL_HANDLE;
            if (auto texture = dynamic_cast<VulkanTexture*>(resource)) {
                targetImage = texture->GetHandle();
            } else {
                LOG_ERROR("Unsupported resource type for depth stencil view!");
                return nullptr;
            }

            if (targetImage == VK_NULL_HANDLE) {
                LOG_ERROR("Failed to get target image for depth stencil view!");
                return nullptr;
            }

            // 创建图像视图
            VkImageViewCreateInfo viewInfo = {};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = targetImage;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = ConvertToVkFormat(desc.Format);

            // 设置深度和模板方面
            viewInfo.subresourceRange.aspectMask = 0;
            if (IsDepthFormat(desc.Format)) {
                viewInfo.subresourceRange.aspectMask |=
                    VK_IMAGE_ASPECT_DEPTH_BIT;
            }
            if (IsStencilFormat(desc.Format)) {
                viewInfo.subresourceRange.aspectMask |=
                    VK_IMAGE_ASPECT_STENCIL_BIT;
            }

            viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            VkImageView imageView;
            if (vkCreateImageView(Device, &viewInfo, nullptr, &imageView) !=
                VK_SUCCESS) {
                LOG_ERROR("Failed to create depth stencil image view!");
                return nullptr;
            }

            // 创建深度模板视图描述符
            VkDescriptorImageInfo descriptorInfo = {};
            descriptorInfo.imageLayout =
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            descriptorInfo.imageView = imageView;
            descriptorInfo.sampler = VK_NULL_HANDLE;  // DSV不需要采样器

            // 创建VulkanDepthStencilView对象并初始化
            VulkanDepthStencilView* vulkanDSV =
                new VulkanDepthStencilView(this, desc);
            if (vulkanDSV) {
                vulkanDSV->SetResource(resource);
                vulkanDSV->SetHandle(imageView, descriptorInfo);
            }
            return vulkanDSV;
        }

        // 辅助函数：检查是否是深度格式
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

        // 辅助函数：检查是否是模板格式
        bool VulkanDevice::IsStencilFormat(EPixelFormat format) const {
            switch (format) {
                case EPixelFormat::D24_UNORM_S8_UINT:
                case EPixelFormat::D32_FLOAT_S8X24_UINT:
                    return true;
                default:
                    return false;
            }
        }

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

        IRHIShaderResourceView* VulkanDevice::CreateShaderResourceView(
            IRHIResource* resource, const ShaderResourceViewDesc& desc) {
            if (!resource) {
                LOG_ERROR(
                    "Invalid resource provided for shader resource view "
                    "creation!");
                return nullptr;
            }

            VkImage targetImage = VK_NULL_HANDLE;

            // 获取目标图像
            if (auto texture = dynamic_cast<VulkanTexture*>(resource)) {
                targetImage = texture->GetHandle();
            } else {
                LOG_ERROR(
                    "Unsupported resource type for shader resource view!");
                return nullptr;
            }

            if (targetImage == VK_NULL_HANDLE) {
                LOG_ERROR(
                    "Failed to get target image for shader resource view!");
                return nullptr;
            }

            // 创建图像视图
            VkImageViewCreateInfo viewInfo = {};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = targetImage;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = ConvertToVkFormat(desc.Format);
            viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = desc.MostDetailedMip;
            viewInfo.subresourceRange.levelCount = desc.MipLevels;
            viewInfo.subresourceRange.baseArrayLayer = desc.FirstArraySlice;
            viewInfo.subresourceRange.layerCount = desc.ArraySize;

            VkImageView imageView;
            if (vkCreateImageView(Device, &viewInfo, nullptr, &imageView) !=
                VK_SUCCESS) {
                LOG_ERROR("Failed to create shader resource image view!");
                return nullptr;
            }

            // 创建采样器
            VkSamplerCreateInfo samplerInfo = {};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.minFilter = VK_FILTER_LINEAR;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.anisotropyEnable = VK_TRUE;
            samplerInfo.maxAnisotropy = 16.0f;
            samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;
            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.mipLodBias = 0.0f;
            samplerInfo.minLod = 0.0f;
            samplerInfo.maxLod = static_cast<float>(desc.MipLevels);

            VkSampler sampler;
            if (vkCreateSampler(Device, &samplerInfo, nullptr, &sampler) !=
                VK_SUCCESS) {
                vkDestroyImageView(Device, imageView, nullptr);
                LOG_ERROR("Failed to create texture sampler!");
                return nullptr;
            }

            // 创建描述符信息
            VkDescriptorImageInfo descriptorInfo = {};
            descriptorInfo.imageLayout =
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            descriptorInfo.imageView = imageView;
            descriptorInfo.sampler = sampler;

            // 创建VulkanShaderResourceView对象并初始化
            // 创建VulkanShaderResourceView对象
            // 构造函数会自动调用CreateView
            return new VulkanShaderResourceView(this, resource, desc);
        }
        // 创建无序访问视图
        IRHIUnorderedAccessView* VulkanDevice::CreateUnorderedAccessView(
            IRHIResource* resource, const UnorderedAccessViewDesc& desc) {
            if (!resource) {
                LOG_ERROR(
                    "Invalid resource provided for unordered access view "
                    "creation!");
                return nullptr;
            }

            VkImage targetImage = VK_NULL_HANDLE;
            VkBuffer targetBuffer = VK_NULL_HANDLE;

            // 根据资源类型获取目标
            if (auto texture = dynamic_cast<VulkanTexture*>(resource)) {
                targetImage = texture->GetHandle();
            } else if (auto buffer = dynamic_cast<VulkanBuffer*>(resource)) {
                targetBuffer = buffer->GetHandle();
            } else {
                LOG_ERROR(
                    "Unsupported resource type for unordered access view!");
                return nullptr;
            }

            if (targetImage == VK_NULL_HANDLE &&
                targetBuffer == VK_NULL_HANDLE) {
                LOG_ERROR(
                    "Failed to get target resource for unordered access view!");
                return nullptr;
            }

            if (targetImage) {
                // 创建图像视图
                VkImageViewCreateInfo viewInfo = {};
                viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                viewInfo.image = targetImage;
                viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                viewInfo.format = ConvertToVkFormat(desc.Format);
                viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
                viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
                viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
                viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
                viewInfo.subresourceRange.aspectMask =
                    VK_IMAGE_ASPECT_COLOR_BIT;
                viewInfo.subresourceRange.baseMipLevel = desc.MipSlice;
                viewInfo.subresourceRange.levelCount = 1;
                viewInfo.subresourceRange.baseArrayLayer = desc.FirstArraySlice;
                viewInfo.subresourceRange.layerCount = desc.ArraySize;

                VkImageView imageView;
                if (vkCreateImageView(Device, &viewInfo, nullptr, &imageView) !=
                    VK_SUCCESS) {
                    LOG_ERROR("Failed to create unordered access image view!");
                    return nullptr;
                }

                // 创建VulkanUnorderedAccessView对象
                VulkanUnorderedAccessView* vulkanUAV =
                    new VulkanUnorderedAccessView(this, resource, desc);
                if (vulkanUAV) {
                    vulkanUAV->SetResource(resource);
                    VkDescriptorImageInfo descriptorInfo = {};
                    descriptorInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                    descriptorInfo.imageView = imageView;
                    descriptorInfo.sampler = VK_NULL_HANDLE;
                    vulkanUAV->SetHandle(imageView, descriptorInfo);
                }
                return vulkanUAV;
            } else {
                // 创建缓冲区视图
                VkBufferViewCreateInfo viewInfo = {};
                viewInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
                viewInfo.buffer = targetBuffer;
                viewInfo.format = ConvertToVkFormat(desc.Format);
                viewInfo.offset = desc.FirstElement * desc.ElementSize;
                viewInfo.range = desc.NumElements * desc.ElementSize;

                VkBufferView bufferView;
                if (vkCreateBufferView(
                        Device, &viewInfo, nullptr, &bufferView) !=
                    VK_SUCCESS) {
                    LOG_ERROR("Failed to create unordered access buffer view!");
                    return nullptr;
                }

                // 创建VulkanUnorderedAccessView对象
                VulkanUnorderedAccessView* vulkanUAV =
                    new VulkanUnorderedAccessView(this, resource, desc);
                if (vulkanUAV) {
                    vulkanUAV->SetResource(resource);
                    vulkanUAV->SetBufferView(bufferView);
                }
                return vulkanUAV;
            }
        }

        // 创建采样器状态
        IRHISamplerState* VulkanDevice::CreateSamplerState(
            const SamplerDesc& desc) {
            VkSamplerCreateInfo samplerInfo = {};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

            // 设置过滤模式
            switch (desc.Filter) {
                case ESamplerFilter::MinMagMipPoint:
                    samplerInfo.magFilter = VK_FILTER_NEAREST;
                    samplerInfo.minFilter = VK_FILTER_NEAREST;
                    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
                    break;
                case ESamplerFilter::MinMagMipLinear:
                    samplerInfo.magFilter = VK_FILTER_LINEAR;
                    samplerInfo.minFilter = VK_FILTER_LINEAR;
                    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
                    break;
                default:
                    samplerInfo.magFilter = VK_FILTER_LINEAR;
                    samplerInfo.minFilter = VK_FILTER_LINEAR;
                    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            }

            // 设置寻址模式
            auto convertAddressMode = [](ESamplerAddressMode mode) {
                switch (mode) {
                    case ESamplerAddressMode::Wrap:
                        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
                    case ESamplerAddressMode::Mirror:
                        return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
                    case ESamplerAddressMode::Clamp:
                        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                    case ESamplerAddressMode::Border:
                        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
                    default:
                        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
                }
            };

            samplerInfo.addressModeU = convertAddressMode(desc.AddressU);
            samplerInfo.addressModeV = convertAddressMode(desc.AddressV);
            samplerInfo.addressModeW = convertAddressMode(desc.AddressW);

            // 各向异性过滤
            samplerInfo.anisotropyEnable =
                desc.MaxAnisotropy > 1 ? VK_TRUE : VK_FALSE;
            samplerInfo.maxAnisotropy = static_cast<float>(desc.MaxAnisotropy);

            // MipMap设置
            samplerInfo.mipLodBias = desc.MipLODBias;
            samplerInfo.minLod = desc.MinLOD;
            samplerInfo.maxLod = desc.MaxLOD;

            // 比较函数
            samplerInfo.compareEnable =
                desc.ComparisonFunc != ECompareFunction::Never ? VK_TRUE
                                                               : VK_FALSE;
            switch (desc.ComparisonFunc) {
                case ECompareFunction::Less:
                    samplerInfo.compareOp = VK_COMPARE_OP_LESS;
                    break;
                case ECompareFunction::LessEqual:
                    samplerInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
                    break;
                case ECompareFunction::Greater:
                    samplerInfo.compareOp = VK_COMPARE_OP_GREATER;
                    break;
                case ECompareFunction::GreaterEqual:
                    samplerInfo.compareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
                    break;
                case ECompareFunction::Equal:
                    samplerInfo.compareOp = VK_COMPARE_OP_EQUAL;
                    break;
                case ECompareFunction::NotEqual:
                    samplerInfo.compareOp = VK_COMPARE_OP_NOT_EQUAL;
                    break;
                case ECompareFunction::Always:
                    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
                    break;
                default:
                    samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
            }

            // 边框颜色
            samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;

            VkSampler sampler;
            if (vkCreateSampler(Device, &samplerInfo, nullptr, &sampler) !=
                VK_SUCCESS) {
                LOG_ERROR("Failed to create sampler!");
                return nullptr;
            }

            // 创建VulkanSamplerState对象
            VulkanSamplerState* vulkanSampler =
                new VulkanSamplerState(this, desc);
            if (vulkanSampler) {
                vulkanSampler->SetHandle(sampler);
            }
            return vulkanSampler;
        }

        // 创建交换链
        IRHISwapChain* VulkanDevice::CreateSwapChain(
            const SwapChainDesc& desc) {
            // 创建VulkanSwapChain对象
            VulkanSwapChain* swapChain = new VulkanSwapChain(this, desc);
            if (!swapChain->Initialize()) {
                delete swapChain;
                return nullptr;
            }
            return swapChain;
        }

        // 创建事件
        IRHIEvent* VulkanDevice::CreateEvent() {
            VkEventCreateInfo eventInfo = {};
            eventInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;

            VkEvent event;
            if (vkCreateEvent(Device, &eventInfo, nullptr, &event) !=
                VK_SUCCESS) {
                LOG_ERROR("Failed to create event!");
                return nullptr;
            }

            // 创建VulkanEvent对象
            return new VulkanEvent(this, event);
        }

        // 创建时间线信号量
        IRHITimelineSemaphore* VulkanDevice::CreateTimelineSemaphore(
            const TimelineSemaphoreDesc& desc) {
            VkSemaphoreTypeCreateInfo timelineCreateInfo = {};
            timelineCreateInfo.sType =
                VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
            timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
            timelineCreateInfo.initialValue = desc.InitialValue;

            VkSemaphoreCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            createInfo.pNext = &timelineCreateInfo;

            VkSemaphore semaphore;
            if (vkCreateSemaphore(Device, &createInfo, nullptr, &semaphore) !=
                VK_SUCCESS) {
                LOG_ERROR("Failed to create timeline semaphore!");
                return nullptr;
            }

            // 创建VulkanTimelineSemaphore对象
            return new VulkanTimelineSemaphore(this, desc, semaphore);
        }

        // 创建着色器反馈缓冲区
        IRHIShaderFeedbackBuffer* VulkanDevice::CreateShaderFeedbackBuffer(
            const ShaderFeedbackDesc& desc) {
            VkBufferCreateInfo bufferInfo = {};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = desc.SizeInBytes;
            bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                               VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VkBuffer buffer;
            if (vkCreateBuffer(Device, &bufferInfo, nullptr, &buffer) !=
                VK_SUCCESS) {
                LOG_ERROR("Failed to create shader feedback buffer!");
                return nullptr;
            }

            // 分配内存
            VkMemoryRequirements memRequirements;
            vkGetBufferMemoryRequirements(Device, buffer, &memRequirements);

            VkMemoryAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = PhysicalDevice->GetMemoryTypeIndex(
                memRequirements.memoryTypeBits,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            VkDeviceMemory bufferMemory;
            if (vkAllocateMemory(Device, &allocInfo, nullptr, &bufferMemory) !=
                VK_SUCCESS) {
                vkDestroyBuffer(Device, buffer, nullptr);
                LOG_ERROR("Failed to allocate shader feedback buffer memory!");
                return nullptr;
            }

            if (vkBindBufferMemory(Device, buffer, bufferMemory, 0) !=
                VK_SUCCESS) {
                vkDestroyBuffer(Device, buffer, nullptr);
                vkFreeMemory(Device, bufferMemory, nullptr);
                LOG_ERROR("Failed to bind shader feedback buffer memory!");
                return nullptr;
            }

            // 创建VulkanShaderFeedbackBuffer对象
            return new VulkanShaderFeedbackBuffer(
                this, desc, buffer, bufferMemory);
        }

        // 创建谓词
        IRHIPredicate* VulkanDevice::CreatePredicate(
            const PredicateDesc& desc) {
            VkQueryPoolCreateInfo queryPoolInfo = {};
            queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
            queryPoolInfo.queryType = VK_QUERY_TYPE_OCCLUSION;
            queryPoolInfo.queryCount = 1;

            VkQueryPool queryPool;
            if (vkCreateQueryPool(
                    Device, &queryPoolInfo, nullptr, &queryPool) !=
                VK_SUCCESS) {
                LOG_ERROR("Failed to create predicate query pool!");
                return nullptr;
            }

            // 创建VulkanPredicate对象
            return new VulkanPredicate(this, desc, queryPool);
        }

        // 提交命令列表
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

        // 等待GPU
        void VulkanDevice::WaitForGPU() { vkDeviceWaitIdle(Device); }

        // 发送GPU信号
        void VulkanDevice::SignalGPU() {
            // 创建一个栅栏来同步
            VkFenceCreateInfo fenceInfo = {};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

            VkFence fence;
            if (vkCreateFence(Device, &fenceInfo, nullptr, &fence) !=
                VK_SUCCESS) {
                LOG_ERROR("Failed to create fence for GPU signaling!");
                return;
            }

            // 提交一个空的命令缓冲区来触发栅栏
            VkSubmitInfo submitInfo = {};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

            if (vkQueueSubmit(GraphicsQueue, 1, &submitInfo, fence) !=
                VK_SUCCESS) {
                LOG_ERROR(
                    "Failed to submit empty command buffer for GPU signaling!");
                vkDestroyFence(Device, fence, nullptr);
                return;
            }

            // 等待栅栏
            vkWaitForFences(Device, 1, &fence, VK_TRUE, UINT64_MAX);
            vkDestroyFence(Device, fence, nullptr);
        }

        // 获取特性级别
        ERHIFeatureLevel VulkanDevice::GetFeatureLevel() const {
            return PhysicalDevice->GetFeatureLevel();
        }

        // 检查是否支持光线追踪
        bool VulkanDevice::SupportsRayTracing() const {
            // 检查设备是否支持光线追踪扩展
            VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtFeatures = {};
            rtFeatures.sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;

            VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
            deviceFeatures2.sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            deviceFeatures2.pNext = &rtFeatures;

            vkGetPhysicalDeviceFeatures2(PhysicalDevice->GetHandle(),
                                         &deviceFeatures2);

            return rtFeatures.rayTracingPipeline == VK_TRUE;
        }

        // 检查是否支持可变速率着色
        bool VulkanDevice::SupportsVariableRateShading() const {
            // 检查设备是否支持VRS扩展
            VkPhysicalDeviceFragmentShadingRateFeaturesKHR vrsFeatures = {};
            vrsFeatures.sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR;

            VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
            deviceFeatures2.sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            deviceFeatures2.pNext = &vrsFeatures;

            vkGetPhysicalDeviceFeatures2(PhysicalDevice->GetHandle(),
                                         &deviceFeatures2);

            return vrsFeatures.pipelineFragmentShadingRate == VK_TRUE;
        }

        // 检查是否支持网格着色器
        bool VulkanDevice::SupportsMeshShaders() const {
            // 检查设备是否支持网格着色器扩展
            VkPhysicalDeviceMeshShaderFeaturesNV meshFeatures = {};
            meshFeatures.sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV;

            VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
            deviceFeatures2.sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            deviceFeatures2.pNext = &meshFeatures;

            vkGetPhysicalDeviceFeatures2(PhysicalDevice->GetHandle(),
                                         &deviceFeatures2);

            return meshFeatures.meshShader == VK_TRUE;
        }

        // 获取总显存大小
        uint64 VulkanDevice::GetTotalVideoMemory() const {
            VkPhysicalDeviceMemoryProperties memProperties;
            vkGetPhysicalDeviceMemoryProperties(PhysicalDevice->GetHandle(),
                                                &memProperties);

            uint64 totalMemory = 0;
            for (uint32_t i = 0; i < memProperties.memoryHeapCount; i++) {
                if (memProperties.memoryHeaps[i].flags &
                    VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                    totalMemory += memProperties.memoryHeaps[i].size;
                }
            }
            return totalMemory;
        }

        // 获取可用显存大小
        uint64 VulkanDevice::GetAvailableVideoMemory() const {
            // 注意：Vulkan没有直接的API来获取可用内存
            // 这里返回一个估计值，实际实现可能需要自己跟踪内存使用情况
            return GetTotalVideoMemory() -
                   PhysicalDevice->GetMemoryAllocator()->GetStats().CurrentUsed;
        }

        // 获取统计信息
        void VulkanDevice::GetStats(RHIStats& stats) const {
            // 填充统计信息
            stats = {};  // 清零所有字段

            // 获取内存统计
            const auto& memStats =
                PhysicalDevice->GetMemoryAllocator()->GetStats();
            stats.TotalMemoryAllocated = memStats.TotalAllocated;
            stats.CurrentMemoryUsed = memStats.CurrentUsed;

            // 其他统计信息可以根据需要添加
        }

        // 开始帧
        void VulkanDevice::BeginFrame() {
            // 实现帧开始逻辑
            // 可以在这里重置命令池、更新统计信息等
        }

        // 结束帧
        void VulkanDevice::EndFrame() {
            // 实现帧结束逻辑
            // 可以在这里提交命令、展示交换链等
        }

        // 设置调试名称
        void VulkanDevice::SetDebugName(IRHIResource* resource,
                                        const char* name) {
            if (!resource || !name) {
                return;
            }

            // 获取资源的Vulkan句柄和类型
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
            // 可以添加其他资源类型的处理...

            if (handle && objectType != VK_OBJECT_TYPE_UNKNOWN) {
                SetDebugName(objectType, handle, name);
            }
        }

        // Vulkan特定的调试名称设置
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
