
#include "VulkanPhysicalDevice.h"

#include <algorithm>
#include <vector>

#include "Core/Public/Log/LogSystem.h"

namespace Engine {
    namespace RHI {

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
            if (props.apiVersion >= VK_API_VERSION_1_2) {
                return ERHIFeatureLevel::SM6;
            } else if (props.apiVersion >= VK_API_VERSION_1_1) {
                return ERHIFeatureLevel::SM5;
            } else {
                return ERHIFeatureLevel::ES3_1;
            }
        }

        uint32_t VulkanPhysicalDevice::GetMemoryTypeIndex(
            uint32_t typeFilter, VkMemoryPropertyFlags properties) const {
            VkPhysicalDeviceMemoryProperties memProperties;
            vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &memProperties);

            for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
                if ((typeFilter & (1 << i)) &&
                    (memProperties.memoryTypes[i].propertyFlags & properties) ==
                        properties) {
                    return i;
                }
            }

            LOG_ERROR("Failed to find suitable memory type!");
            return 0;
        }

    }  // namespace RHI
}  // namespace Engine
